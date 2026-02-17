/*
 * deploy.c - Cross-platform GitHub Pages deploy tool
 * Pushes the contents of the build/ directory to a GitHub Pages repository.
 *
 * Reads the target repo URL from ../deploy.conf (or deploy.conf in cwd).
 * Can also accept the repo URL as a command-line argument.
 *
 * Build:
 *   gcc -O2 -o deploy deploy.c                          (Linux)
 *   cc -O2 -o deploy deploy.c --sysroot="$(xcrun --show-sdk-path)"  (macOS)
 *   cl deploy.c /Fe:deploy.exe                           (Windows MSVC)
 *   gcc -O2 -o deploy.exe deploy.c                       (Windows MinGW)
 *
 * Usage:
 *   ./deploy                      (reads repo URL from deploy.conf)
 *   ./deploy <repo-url>           (uses provided URL, saves to deploy.conf)
 *   ./deploy --config             (print current config)
 *   ./deploy --set <repo-url>     (save URL without deploying)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #include <io.h>
  #define PATH_SEP '\\'
  #define MKDIR(d) _mkdir(d)
  #define STAT _stat
  #define S_ISDIR(m) (((m) & _S_IFDIR) != 0)
  #define S_ISREG(m) (((m) & _S_IFREG) != 0)
  #define popen _popen
  #define pclose _pclose
  #define snprintf _snprintf
#else
  #include <unistd.h>
  #include <dirent.h>
  #include <errno.h>
  #define PATH_SEP '/'
  #define MKDIR(d) mkdir(d, 0755)
  #define STAT stat
#endif

#define MAX_URL 1024
#define MAX_PATH_LEN 2048
#define MAX_CMD 4096

/* ---- Utility ---- */

static void trim(char *s) {
    /* Trim trailing whitespace/newlines */
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' ||
                       s[len-1] == ' '  || s[len-1] == '\t')) {
        s[--len] = '\0';
    }
    /* Trim leading whitespace */
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static int file_exists(const char *path) {
    struct STAT st;
    return STAT(path, &st) == 0;
}

static int dir_exists(const char *path) {
    struct STAT st;
    return STAT(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int run_cmd(const char *cmd) {
    printf("  > %s\n", cmd);
    int rc = system(cmd);
    return rc;
}

static int run_cmd_quiet(const char *cmd) {
    return system(cmd);
}

/* ---- Config file ---- */

static int find_config(char *out, int out_size) {
    /* Try ./deploy.conf first, then ../deploy.conf */
    if (file_exists("deploy.conf")) {
        snprintf(out, out_size, "deploy.conf");
        return 1;
    }
    if (file_exists("../deploy.conf")) {
        snprintf(out, out_size, "../deploy.conf");
        return 1;
    }
    return 0;
}

static int read_config(char *url, int url_size) {
    char config_path[MAX_PATH_LEN];
    if (!find_config(config_path, sizeof(config_path))) {
        return 0;
    }

    FILE *f = fopen(config_path, "r");
    if (!f) return 0;

    url[0] = '\0';
    char line[MAX_URL];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0') continue;
        /* Look for repo= or REPO= */
        if (strncmp(line, "repo=", 5) == 0) {
            strncpy(url, line + 5, url_size - 1);
            url[url_size - 1] = '\0';
            trim(url);
            break;
        }
        if (strncmp(line, "REPO=", 5) == 0) {
            strncpy(url, line + 5, url_size - 1);
            url[url_size - 1] = '\0';
            trim(url);
            break;
        }
        /* If no key=value, treat first non-comment line as the URL */
        if (url[0] == '\0') {
            strncpy(url, line, url_size - 1);
            url[url_size - 1] = '\0';
            break;
        }
    }

    fclose(f);
    return url[0] != '\0';
}

static int write_config(const char *url, const char *config_path) {
    FILE *f = fopen(config_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot write to %s\n", config_path);
        return 0;
    }
    fprintf(f, "# deploy.conf - GitHub Pages deploy target\n");
    fprintf(f, "# This repo will receive the built portfolio files.\n");
    fprintf(f, "repo=%s\n", url);
    fclose(f);
    printf("Saved deploy config: %s\n", config_path);
    return 1;
}

/* ---- Build directory discovery ---- */

static int find_build_dir(char *out, int out_size) {
    if (dir_exists("build")) {
        snprintf(out, out_size, "build");
        return 1;
    }
    if (dir_exists("../build")) {
        snprintf(out, out_size, "../build");
        return 1;
    }
    return 0;
}

/* ---- Copy files from build dir to staging dir ---- */

#ifdef _WIN32
static int copy_build_files(const char *build_dir, const char *dest_dir) {
    char cmd[MAX_CMD];
    /* Use xcopy on Windows */
    snprintf(cmd, sizeof(cmd), "xcopy /E /Y /Q \"%s\\*\" \"%s\\\"", build_dir, dest_dir);
    return run_cmd(cmd);
}
#else
static int copy_build_files(const char *build_dir, const char *dest_dir) {
    char cmd[MAX_CMD];
    /* Use cp on Unix */
    snprintf(cmd, sizeof(cmd), "cp -R \"%s\"/. \"%s\"/", build_dir, dest_dir);
    return run_cmd(cmd);
}
#endif

/* ---- Get temp directory ---- */

static void get_temp_dir(char *out, int out_size) {
#ifdef _WIN32
    const char *tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "C:\\Temp";
    snprintf(out, out_size, "%s\\portfolio-deploy", tmp);
#else
    const char *tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    snprintf(out, out_size, "%s/portfolio-deploy", tmp);
#endif
}

/* ---- Remove directory recursively ---- */

static void remove_dir(const char *path) {
    char cmd[MAX_CMD];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "rmdir /S /Q \"%s\" 2>nul", path);
#else
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
#endif
    run_cmd_quiet(cmd);
}

/* ---- Deploy ---- */

static int deploy(const char *repo_url) {
    char build_dir[MAX_PATH_LEN];
    char staging[MAX_PATH_LEN];
    char cmd[MAX_CMD];

    printf("\n--- Deploy to GitHub Pages ---\n\n");

    /* 1. Find build directory */
    if (!find_build_dir(build_dir, sizeof(build_dir))) {
        fprintf(stderr, "Error: No build/ directory found.\n");
        fprintf(stderr, "Run the build tool first to generate the portfolio files.\n");
        return 1;
    }
    printf("Build directory: %s\n", build_dir);
    printf("Target repo: %s\n\n", repo_url);

    /* 2. Create staging directory */
    get_temp_dir(staging, sizeof(staging));
    remove_dir(staging);
    MKDIR(staging);

    if (!dir_exists(staging)) {
        fprintf(stderr, "Error: Could not create staging directory: %s\n", staging);
        return 1;
    }

    /* 3. Initialize git repo in staging */
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git init && git checkout -b main", staging);
    if (run_cmd(cmd) != 0) {
        fprintf(stderr, "Error: git init failed.\n");
        remove_dir(staging);
        return 1;
    }

    /* 4. Copy build files to staging */
    printf("\nCopying build files...\n");
    if (copy_build_files(build_dir, staging) != 0) {
        fprintf(stderr, "Error: Failed to copy build files.\n");
        remove_dir(staging);
        return 1;
    }

    /* 5. Create a minimal .nojekyll file (required for GitHub Pages) */
    {
        char nojekyll[MAX_PATH_LEN];
        snprintf(nojekyll, sizeof(nojekyll), "%s%c.nojekyll", staging, PATH_SEP);
        FILE *f = fopen(nojekyll, "w");
        if (f) fclose(f);
    }

    /* 5b. Create CNAME file if custom domain is configured */
    {
        char cname_src[MAX_PATH_LEN];
        char cname_dst[MAX_PATH_LEN];
        /* Check for CNAME in build dir first, then project root */
        int found = 0;
        snprintf(cname_src, sizeof(cname_src), "%s%cCNAME", build_dir, PATH_SEP);
        if (file_exists(cname_src)) { found = 1; }
        if (!found) {
            if (file_exists("CNAME")) {
                snprintf(cname_src, sizeof(cname_src), "CNAME");
                found = 1;
            } else if (file_exists("../CNAME")) {
                snprintf(cname_src, sizeof(cname_src), "../CNAME");
                found = 1;
            }
        }
        snprintf(cname_dst, sizeof(cname_dst), "%s%cCNAME", staging, PATH_SEP);
        if (found) {
            /* Copy existing CNAME */
            FILE *src = fopen(cname_src, "r");
            FILE *dst = fopen(cname_dst, "w");
            if (src && dst) {
                char cbuf[512];
                size_t r;
                while ((r = fread(cbuf, 1, sizeof(cbuf), src)) > 0) {
                    fwrite(cbuf, 1, r, dst);
                }
                printf("CNAME: copied from %s\n", cname_src);
            }
            if (src) fclose(src);
            if (dst) fclose(dst);
        } else {
            /* Check deploy.conf for domain= line */
            char config_path[MAX_PATH_LEN];
            char domain[256] = {0};
            if (find_config(config_path, sizeof(config_path))) {
                FILE *cf = fopen(config_path, "r");
                if (cf) {
                    char line[MAX_URL];
                    while (fgets(line, sizeof(line), cf)) {
                        trim(line);
                        if (strncmp(line, "domain=", 7) == 0) {
                            strncpy(domain, line + 7, sizeof(domain) - 1);
                            trim(domain);
                            break;
                        }
                    }
                    fclose(cf);
                }
            }
            if (domain[0]) {
                FILE *dst = fopen(cname_dst, "w");
                if (dst) {
                    fprintf(dst, "%s\n", domain);
                    fclose(dst);
                    printf("CNAME: %s\n", domain);
                }
            }
        }
    }

    /* 6. Stage all files */
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git add -A", staging);
    if (run_cmd(cmd) != 0) {
        fprintf(stderr, "Error: git add failed.\n");
        remove_dir(staging);
        return 1;
    }

    /* 7. Commit */
    snprintf(cmd, sizeof(cmd),
        "cd \"%s\" && git commit -m \"Deploy portfolio\"",
        staging);
    if (run_cmd(cmd) != 0) {
        fprintf(stderr, "Error: git commit failed.\n");
        remove_dir(staging);
        return 1;
    }

    /* 8. Set remote and force push to main branch */
    snprintf(cmd, sizeof(cmd),
        "cd \"%s\" && git remote add origin \"%s\"",
        staging, repo_url);
    run_cmd(cmd);

    printf("\nPushing to %s ...\n", repo_url);
    snprintf(cmd, sizeof(cmd),
        "cd \"%s\" && git push -f origin main",
        staging);
    int push_rc = run_cmd(cmd);

    if (push_rc != 0) {
        /* Try gh-pages branch as fallback */
        printf("\nTrying gh-pages branch...\n");
        snprintf(cmd, sizeof(cmd),
            "cd \"%s\" && git push -f origin main:gh-pages",
            staging);
        push_rc = run_cmd(cmd);
    }

    /* 9. Cleanup */
    remove_dir(staging);

    if (push_rc != 0) {
        fprintf(stderr, "\nError: Push failed. Check your git credentials and repo URL.\n");
        return 1;
    }

    printf("\n--- Deploy complete ---\n");

    /* Check for custom domain first */
    {
        char config_path[MAX_PATH_LEN];
        char domain[256] = {0};
        if (find_config(config_path, sizeof(config_path))) {
            FILE *cf = fopen(config_path, "r");
            if (cf) {
                char line[MAX_URL];
                while (fgets(line, sizeof(line), cf)) {
                    trim(line);
                    if (strncmp(line, "domain=", 7) == 0) {
                        strncpy(domain, line + 7, sizeof(domain) - 1);
                        trim(domain);
                        break;
                    }
                }
                fclose(cf);
            }
        }
        if (domain[0]) {
            printf("Your site should be live at: https://%s/\n", domain);
            printf("(It may take a minute or two for GitHub Pages to update.)\n");
            return 0;
        }
    }

    /* Fall back to deriving the pages URL from the repo URL */
    {
        char pages_url[MAX_URL] = {0};
        const char *gh = strstr(repo_url, "github.com/");
        if (gh) {
            const char *after = gh + 11;
            char user[256] = {0};
            char repo[256] = {0};
            int i = 0;
            while (after[i] && after[i] != '/' && i < 255) {
                user[i] = after[i];
                i++;
            }
            user[i] = '\0';
            if (after[i] == '/') {
                i++;
                int j = 0;
                while (after[i] && after[i] != '/' && after[i] != '.' && j < 255) {
                    repo[j++] = after[i++];
                }
                repo[j] = '\0';
            }
            if (user[0] && repo[0]) {
                char user_pages[512];
                snprintf(user_pages, sizeof(user_pages), "%s.github.io", user);
                if (strcmp(repo, user_pages) == 0) {
                    snprintf(pages_url, sizeof(pages_url),
                        "https://%s.github.io/", user);
                } else {
                    snprintf(pages_url, sizeof(pages_url),
                        "https://%s.github.io/%s/", user, repo);
                }
                printf("Your site should be live at: %s\n", pages_url);
                printf("(It may take a minute or two for GitHub Pages to update.)\n");
            }
        }
    }

    return 0;
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    char repo_url[MAX_URL] = {0};
    char config_path[MAX_PATH_LEN] = {0};

    /* Determine config file path (prefer parent dir for when run from deploy/) */
    if (file_exists("deploy.conf")) {
        snprintf(config_path, sizeof(config_path), "deploy.conf");
    } else if (file_exists("../deploy.conf")) {
        snprintf(config_path, sizeof(config_path), "../deploy.conf");
    } else {
        /* Default: write to parent dir (project root) */
        snprintf(config_path, sizeof(config_path), "../deploy.conf");
    }

    /* Parse arguments */
    if (argc == 2) {
        if (strcmp(argv[1], "--config") == 0) {
            if (read_config(repo_url, sizeof(repo_url))) {
                printf("Deploy target: %s\n", repo_url);
            } else {
                printf("No deploy.conf found. Run: deploy <repo-url>\n");
            }
            return 0;
        }
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Usage:\n");
            printf("  deploy                     Deploy using saved repo URL\n");
            printf("  deploy <repo-url>          Deploy and save repo URL\n");
            printf("  deploy --set <repo-url>    Save repo URL without deploying\n");
            printf("  deploy --config            Show current config\n");
            return 0;
        }
        /* Treat as repo URL */
        strncpy(repo_url, argv[1], sizeof(repo_url) - 1);
        trim(repo_url);
        write_config(repo_url, config_path);
    } else if (argc == 3 && strcmp(argv[1], "--set") == 0) {
        strncpy(repo_url, argv[2], sizeof(repo_url) - 1);
        trim(repo_url);
        write_config(repo_url, config_path);
        printf("Config saved. Run 'deploy' to push.\n");
        return 0;
    } else if (argc == 1) {
        if (!read_config(repo_url, sizeof(repo_url))) {
            fprintf(stderr, "Error: No deploy.conf found.\n");
            fprintf(stderr, "Usage: deploy <github-pages-repo-url>\n");
            fprintf(stderr, "Example: deploy https://github.com/user/user.github.io.git\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: deploy [<repo-url>] [--set <repo-url>] [--config]\n");
        return 1;
    }

    /* Validate URL */
    if (strlen(repo_url) < 10) {
        fprintf(stderr, "Error: Invalid repo URL: %s\n", repo_url);
        return 1;
    }

    return deploy(repo_url);
}
