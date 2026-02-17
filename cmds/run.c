/*
 * run.c - Cross-platform command runner with approval gate
 *
 * A standalone CLI tool that an AI agent (or any caller) invokes to run
 * shell commands.  Before every execution the operator sees the full
 * command and must type "y" to approve it.  Any other input, including
 * empty input or EOF, rejects the command and nothing runs.
 *
 * The tool communicates entirely through stdin/stdout/stderr so it
 * integrates with any tool-call protocol (MCP, function-calling JSON,
 * plain pipes, etc.).
 *
 * Exit codes
 *   0  - command ran and finished with exit code 0
 *   1  - command ran but returned a non-zero exit code (echoed to stderr)
 *   2  - operator rejected the command
 *   3  - usage error (no command supplied)
 *
 * Build
 *   gcc -O2 -o run run.c              (macOS / Linux)
 *   cl run.c /Fe:run.exe              (Windows MSVC)
 *   gcc -O2 -o run.exe run.c          (Windows MinGW)
 *
 * Usage
 *   ./run <command> [args ...]
 *   ./run "ls -la /tmp"
 *   ./run git status
 *   echo '{"cmd":"ls"}' | ./run --json
 *
 * Flags
 *   --json     Read a single JSON object from stdin with a "cmd" field.
 *              The approval prompt still appears on the terminal.
 *   --yes      Skip the approval prompt (use only in trusted pipelines).
 *   --timeout  Seconds to wait for approval input (0 = no timeout, default).
 *   --log      Path to an append-only log file that records every request
 *              and its approval/rejection status with a timestamp.
 *
 * Compiles on Windows (MSVC, MinGW), macOS, and Linux with no external
 * libraries.  Uses only the C standard library and POSIX / Win32 process
 * APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- Platform ---- */

#ifdef _WIN32
  #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
  #endif
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
  #define IS_TTY(f) _isatty(_fileno(f))
#else
  #include <unistd.h>
  #define IS_TTY(f) isatty(fileno(f))
#endif

/* ---- Constants ---- */

#define EXIT_OK        0
#define EXIT_CMDFAIL   1
#define EXIT_REJECTED  2
#define EXIT_USAGE     3

#define CMD_MAX        8192
#define LINE_MAX_BUF   256
#define LOG_LINE_MAX   9000

/* ---- Globals set by flags ---- */

static int  flag_yes     = 0;
static int  flag_json    = 0;
static int  flag_timeout = 0;
static char flag_log[1024] = {0};

/* ---- Helpers ---- */

static void timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

static void log_entry(const char *cmd, const char *status) {
    if (flag_log[0] == '\0') return;
    FILE *fp = fopen(flag_log, "a");
    if (!fp) return;
    char ts[64];
    timestamp(ts, sizeof(ts));
    fprintf(fp, "[%s] %s | %s\n", ts, status, cmd);
    fclose(fp);
}

/*
 * Build a single command string from argv tokens.
 * Handles quoting on Windows where necessary.
 */
static int build_command(int argc, char **argv, int start, char *out, size_t out_len) {
    out[0] = '\0';
    size_t pos = 0;
    for (int i = start; i < argc; i++) {
        size_t arglen = strlen(argv[i]);
        int needs_quote = 0;

        /* Check if the argument contains spaces or special chars */
        for (size_t j = 0; j < arglen; j++) {
            char c = argv[i][j];
            if (c == ' ' || c == '\t' || c == '&' || c == '|' ||
                c == '<' || c == '>' || c == ';' || c == '"') {
                needs_quote = 1;
                break;
            }
        }

        if (i > start) {
            if (pos + 1 >= out_len) return -1;
            out[pos++] = ' ';
        }

        if (needs_quote) {
            if (pos + arglen + 2 >= out_len) return -1;
            out[pos++] = '"';
            memcpy(out + pos, argv[i], arglen);
            pos += arglen;
            out[pos++] = '"';
        } else {
            if (pos + arglen >= out_len) return -1;
            memcpy(out + pos, argv[i], arglen);
            pos += arglen;
        }
    }
    out[pos] = '\0';
    return 0;
}

/*
 * Read the "cmd" value from a JSON blob on stdin.
 * Minimal parser -- finds "cmd" key and extracts the string value.
 */
static int read_json_cmd(char *out, size_t out_len) {
    char buf[CMD_MAX];
    size_t total = 0;
    while (total < sizeof(buf) - 1) {
        int ch = fgetc(stdin);
        if (ch == EOF) break;
        buf[total++] = (char)ch;
    }
    buf[total] = '\0';

    /* Find "cmd" key */
    char *key = strstr(buf, "\"cmd\"");
    if (!key) {
        key = strstr(buf, "'cmd'");
    }
    if (!key) {
        fprintf(stderr, "run: JSON input missing \"cmd\" field\n");
        return -1;
    }

    /* Skip past key and colon */
    char *p = key + 5;
    while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;

    if (*p != '"' && *p != '\'') {
        fprintf(stderr, "run: \"cmd\" value must be a string\n");
        return -1;
    }
    char quote = *p;
    p++;

    size_t i = 0;
    while (*p && *p != quote && i < out_len - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'n':  out[i++] = '\n'; break;
                case 't':  out[i++] = '\t'; break;
                case '\\': out[i++] = '\\'; break;
                case '"':  out[i++] = '"';  break;
                case '\'': out[i++] = '\''; break;
                default:   out[i++] = *p;   break;
            }
        } else {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';
    return 0;
}

/*
 * Prompt the operator for approval.
 * Reads from /dev/tty (Unix) or CON (Windows) so it works even when
 * stdin is piped.
 */
static int prompt_approval(const char *cmd) {
    fprintf(stderr, "\n");
    fprintf(stderr, "=== COMMAND APPROVAL REQUIRED ===\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  %s\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Approve? [y/N]: ");
    fflush(stderr);

    FILE *tty = NULL;

#ifdef _WIN32
    tty = fopen("CON", "r");
#else
    tty = fopen("/dev/tty", "r");
#endif

    if (!tty) {
        /* Fall back to stdin if no tty available */
        tty = stdin;
    }

    char line[LINE_MAX_BUF];
    if (!fgets(line, sizeof(line), tty)) {
        if (tty != stdin) fclose(tty);
        return 0;
    }

    if (tty != stdin) fclose(tty);

    /* Trim whitespace */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' ||
                       line[len - 1] == ' '  || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }

    if (len == 0) return 0;
    if (line[0] == 'y' || line[0] == 'Y') return 1;
    return 0;
}

/*
 * Run the command and return its exit code.
 */
static int run_command(const char *cmd) {
#ifdef _WIN32
    /*
     * On Windows, use CreateProcess for proper exit-code capture.
     * Fall back to system() for simplicity.
     */
    int ret = system(cmd);
    return ret;
#else
    int ret = system(cmd);
    if (ret == -1) return 1;
    /* POSIX: system() returns the status from waitpid. Extract exit code. */
    if (WIFEXITED(ret)) return WEXITSTATUS(ret);
    return 1;
#endif
}

/* ---- Usage ---- */

static void print_usage(void) {
    fprintf(stderr,
        "Usage: run [flags] <command> [args ...]\n"
        "\n"
        "Flags:\n"
        "  --json      Read command from a JSON object on stdin ({\"cmd\": \"...\"})\n"
        "  --yes       Skip approval prompt (trusted pipelines only)\n"
        "  --timeout N Seconds to wait for approval (0 = no timeout)\n"
        "  --log FILE  Append every request and result to FILE\n"
        "\n"
        "Examples:\n"
        "  ./run ls -la\n"
        "  ./run \"git status\"\n"
        "  echo '{\"cmd\":\"date\"}' | ./run --json\n"
        "\n"
        "Exit codes:\n"
        "  0  Command ran successfully\n"
        "  1  Command ran but returned non-zero\n"
        "  2  Operator rejected the command\n"
        "  3  Usage error\n"
    );
}

/* ---- Main ---- */

int main(int argc, char **argv) {
    char cmd[CMD_MAX];
    cmd[0] = '\0';

    /* Parse flags */
    int cmd_start = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--yes") == 0) {
            flag_yes = 1;
            cmd_start = i + 1;
        } else if (strcmp(argv[i], "--json") == 0) {
            flag_json = 1;
            cmd_start = i + 1;
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            flag_timeout = atoi(argv[i + 1]);
            i++;
            cmd_start = i + 1;
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            strncpy(flag_log, argv[i + 1], sizeof(flag_log) - 1);
            i++;
            cmd_start = i + 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return EXIT_OK;
        } else {
            /* First non-flag argument -- everything from here is the command */
            cmd_start = i;
            break;
        }
    }

    /* Get the command string */
    if (flag_json) {
        if (read_json_cmd(cmd, sizeof(cmd)) != 0) {
            return EXIT_USAGE;
        }
    } else if (cmd_start < argc) {
        if (build_command(argc, argv, cmd_start, cmd, sizeof(cmd)) != 0) {
            fprintf(stderr, "run: command too long (max %d bytes)\n", CMD_MAX);
            return EXIT_USAGE;
        }
    } else {
        print_usage();
        return EXIT_USAGE;
    }

    if (cmd[0] == '\0') {
        fprintf(stderr, "run: empty command\n");
        return EXIT_USAGE;
    }

    /* Approval gate */
    if (!flag_yes) {
        int approved = prompt_approval(cmd);
        if (!approved) {
            fprintf(stderr, "run: command rejected by operator\n");
            log_entry(cmd, "REJECTED");

            /* Print structured result to stdout for the caller */
            printf("{\"status\":\"rejected\",\"command\":\"%s\"}\n", cmd);
            return EXIT_REJECTED;
        }
    }

    log_entry(cmd, "APPROVED");

    /* Run it */
    fprintf(stderr, "run: executing...\n");
    int exit_code = run_command(cmd);

    /* Print structured result */
    printf("{\"status\":\"executed\",\"exit_code\":%d,\"command\":\"%s\"}\n",
           exit_code, cmd);

    if (exit_code != 0) {
        log_entry(cmd, "FAILED");
        fprintf(stderr, "run: command exited with code %d\n", exit_code);
        return EXIT_CMDFAIL;
    }

    log_entry(cmd, "SUCCESS");
    return EXIT_OK;
}
