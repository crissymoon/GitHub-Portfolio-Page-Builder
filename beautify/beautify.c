/*
 * beautify.c - Cross-platform text and code beautifier
 *
 * Reads text from stdin or a file and outputs formatted, human-readable
 * versions. Supports JSON, HTML, CSS, and plain text. Designed for use
 * with AI agents and automated pipelines.
 *
 * Build:
 *   gcc -O2 -o beautify beautify.c          (macOS / Linux)
 *   cl beautify.c /Fe:beautify.exe          (Windows MSVC)
 *   gcc -O2 -o beautify.exe beautify.c      (Windows MinGW)
 *
 * On macOS if headers are not found:
 *   cc -O2 -o beautify beautify.c --sysroot="$(xcrun --show-sdk-path)"
 *
 * Usage:
 *   echo '{"a":1,"b":[2,3]}' | ./beautify --json
 *   ./beautify --json data.json
 *   ./beautify --html page.html
 *   ./beautify --extract-field explanation response.json
 *   ./beautify --css styles.css
 *
 * Flags:
 *   --json              Format as indented JSON
 *   --html              Format HTML with indentation
 *   --css               Format CSS with indentation
 *   --extract-field KEY Extract a top-level string field from JSON
 *   --indent N          Set indent width (default: 2)
 *   --compact           Minify instead of beautify
 *   --help              Show usage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int indent_width = 2;

/* ---- Read all input into a buffer ---- */

static char *read_all(FILE *f, long *out_len) {
    long capacity = 8192;
    long len = 0;
    char *buf = (char *)malloc(capacity);
    if (!buf) return NULL;
    while (!feof(f)) {
        if (len + 4096 > capacity) {
            capacity *= 2;
            char *tmp = (char *)realloc(buf, capacity);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
        long n = (long)fread(buf + len, 1, 4096, f);
        if (n <= 0) break;
        len += n;
    }
    buf[len] = '\0';
    *out_len = len;
    return buf;
}

static FILE *open_input(const char *path) {
    if (!path || strcmp(path, "-") == 0) {
        return stdin;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        exit(1);
    }
    return f;
}

/* ---- JSON Beautifier ---- */

static void print_indent(int depth) {
    for (int i = 0; i < depth * indent_width; i++) {
        putchar(' ');
    }
}

static void beautify_json(const char *src, long len) {
    int depth = 0;
    int in_string = 0;
    int escaped = 0;

    for (long i = 0; i < len; i++) {
        char c = src[i];

        if (escaped) {
            putchar(c);
            escaped = 0;
            continue;
        }

        if (c == '\\' && in_string) {
            putchar(c);
            escaped = 1;
            continue;
        }

        if (c == '"') {
            putchar(c);
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            putchar(c);
            continue;
        }

        /* Skip whitespace outside strings */
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        }

        if (c == '{' || c == '[') {
            putchar(c);
            /* Check if empty */
            long j = i + 1;
            while (j < len && (src[j] == ' ' || src[j] == '\t' ||
                   src[j] == '\n' || src[j] == '\r')) j++;
            if (j < len && ((c == '{' && src[j] == '}') ||
                            (c == '[' && src[j] == ']'))) {
                putchar(c == '{' ? '}' : ']');
                i = j;
                continue;
            }
            depth++;
            putchar('\n');
            print_indent(depth);
        } else if (c == '}' || c == ']') {
            depth--;
            putchar('\n');
            print_indent(depth);
            putchar(c);
        } else if (c == ',') {
            putchar(',');
            putchar('\n');
            print_indent(depth);
        } else if (c == ':') {
            putchar(':');
            putchar(' ');
        } else {
            putchar(c);
        }
    }
    putchar('\n');
}

static void compact_json(const char *src, long len) {
    int in_string = 0;
    int escaped = 0;

    for (long i = 0; i < len; i++) {
        char c = src[i];

        if (escaped) {
            putchar(c);
            escaped = 0;
            continue;
        }

        if (c == '\\' && in_string) {
            putchar(c);
            escaped = 1;
            continue;
        }

        if (c == '"') {
            putchar(c);
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            putchar(c);
            continue;
        }

        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            putchar(c);
        }
    }
    putchar('\n');
}

/* ---- Extract a top-level string field from JSON ---- */

static void extract_field(const char *src, long len, const char *field) {
    /* Simple extraction: find "field": "value" */
    char needle[512];
    int nlen = snprintf(needle, sizeof(needle), "\"%s\"", field);
    (void)nlen;

    const char *pos = strstr(src, needle);
    if (!pos) {
        fprintf(stderr, "Field '%s' not found.\n", field);
        exit(1);
    }

    /* Skip past the key and colon */
    pos += strlen(needle);
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    if (*pos != ':') {
        fprintf(stderr, "Malformed JSON near field '%s'.\n", field);
        exit(1);
    }
    pos++;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;

    if (*pos != '"') {
        fprintf(stderr, "Field '%s' is not a string value.\n", field);
        exit(1);
    }

    pos++; /* skip opening quote */
    int escaped = 0;
    while (*pos) {
        if (escaped) {
            if (*pos == 'n') putchar('\n');
            else if (*pos == 't') putchar('\t');
            else if (*pos == '"') putchar('"');
            else if (*pos == '\\') putchar('\\');
            else if (*pos == '/') putchar('/');
            else { putchar('\\'); putchar(*pos); }
            escaped = 0;
            pos++;
            continue;
        }
        if (*pos == '\\') {
            escaped = 1;
            pos++;
            continue;
        }
        if (*pos == '"') break;
        putchar(*pos);
        pos++;
    }
    putchar('\n');
}

/* ---- HTML Beautifier ---- */

static void beautify_html(const char *src, long len) {
    int depth = 0;
    int in_tag = 0;
    int is_closing = 0;
    int at_line_start = 1;
    /* Void elements that do not get indented children */
    const char *voids[] = {
        "br", "hr", "img", "input", "meta", "link", "area",
        "base", "col", "embed", "source", "track", "wbr", NULL
    };

    for (long i = 0; i < len; i++) {
        char c = src[i];

        if (c == '<') {
            /* Determine if closing tag */
            is_closing = (i + 1 < len && src[i + 1] == '/');

            if (is_closing && depth > 0) depth--;

            if (!at_line_start) {
                putchar('\n');
            }
            print_indent(depth);
            putchar(c);
            in_tag = 1;
            at_line_start = 0;

            /* Check if this is a void element */
            int is_void = 0;
            if (!is_closing) {
                /* Extract tag name */
                char tagname[64] = {0};
                int ti = 0;
                long j = i + 1;
                while (j < len && src[j] != ' ' && src[j] != '>' &&
                       src[j] != '/' && src[j] != '\n' && ti < 63) {
                    tagname[ti++] = src[j];
                    j++;
                }
                tagname[ti] = '\0';
                for (int v = 0; voids[v]; v++) {
                    if (strcmp(tagname, voids[v]) == 0) {
                        is_void = 1;
                        break;
                    }
                }
                if (!is_void && !is_closing) {
                    depth++;
                }
            }
        } else if (c == '>') {
            putchar(c);
            in_tag = 0;
            at_line_start = 0;
        } else if (c == '\n' || c == '\r') {
            if (in_tag) {
                /* skip newlines inside tags */
            } else {
                at_line_start = 1;
            }
        } else {
            if (at_line_start && !in_tag && (c == ' ' || c == '\t')) {
                continue; /* skip leading whitespace */
            }
            if (at_line_start && !in_tag) {
                putchar('\n');
                print_indent(depth);
                at_line_start = 0;
            }
            putchar(c);
        }
    }
    putchar('\n');
}

/* ---- CSS Beautifier ---- */

static void beautify_css(const char *src, long len) {
    int depth = 0;
    int in_string = 0;
    char string_char = 0;

    for (long i = 0; i < len; i++) {
        char c = src[i];

        /* Handle strings */
        if (in_string) {
            putchar(c);
            if (c == '\\' && i + 1 < len) {
                putchar(src[++i]);
                continue;
            }
            if (c == string_char) in_string = 0;
            continue;
        }
        if (c == '"' || c == '\'') {
            in_string = 1;
            string_char = c;
            putchar(c);
            continue;
        }

        /* Skip whitespace, control our own */
        if (c == '\n' || c == '\r') continue;

        if (c == '{') {
            putchar(' ');
            putchar('{');
            putchar('\n');
            depth++;
            print_indent(depth);
        } else if (c == '}') {
            putchar('\n');
            depth--;
            if (depth < 0) depth = 0;
            print_indent(depth);
            putchar('}');
            putchar('\n');
            if (depth == 0) putchar('\n');
        } else if (c == ';') {
            putchar(';');
            putchar('\n');
            print_indent(depth);
        } else if (c == ' ' || c == '\t') {
            /* Collapse whitespace */
            if (i > 0 && src[i - 1] != ' ' && src[i - 1] != '\t' &&
                src[i - 1] != '\n' && src[i - 1] != '{' &&
                src[i - 1] != ';') {
                putchar(' ');
            }
        } else {
            putchar(c);
        }
    }
    putchar('\n');
}

/* ---- Usage ---- */

static void print_usage(void) {
    printf("Usage: beautify [OPTIONS] [FILE]\n\n");
    printf("Reads from FILE or stdin and outputs formatted text.\n\n");
    printf("Options:\n");
    printf("  --json              Format as indented JSON\n");
    printf("  --html              Format HTML with indentation\n");
    printf("  --css               Format CSS with indentation\n");
    printf("  --extract-field KEY Extract a top-level string field from JSON\n");
    printf("  --indent N          Set indent width (default: 2)\n");
    printf("  --compact           Minify instead of beautify\n");
    printf("  --help              Show this message\n\n");
    printf("Examples:\n");
    printf("  echo '{\"a\":1}' | beautify --json\n");
    printf("  beautify --json data.json\n");
    printf("  beautify --extract-field explanation response.json\n");
    printf("  beautify --css styles.css\n");
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    enum { MODE_JSON, MODE_HTML, MODE_CSS, MODE_EXTRACT } mode = MODE_JSON;
    int compact = 0;
    const char *input_path = NULL;
    const char *extract_key = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            mode = MODE_JSON;
        } else if (strcmp(argv[i], "--html") == 0) {
            mode = MODE_HTML;
        } else if (strcmp(argv[i], "--css") == 0) {
            mode = MODE_CSS;
        } else if (strcmp(argv[i], "--extract-field") == 0) {
            mode = MODE_EXTRACT;
            if (i + 1 < argc) {
                extract_key = argv[++i];
            } else {
                fprintf(stderr, "Error: --extract-field requires a key name.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--indent") == 0) {
            if (i + 1 < argc) {
                indent_width = atoi(argv[++i]);
                if (indent_width < 0) indent_width = 0;
                if (indent_width > 16) indent_width = 16;
            }
        } else if (strcmp(argv[i], "--compact") == 0) {
            compact = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] != '-') {
            input_path = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    FILE *f = open_input(input_path);
    long len = 0;
    char *src = read_all(f, &len);
    if (f != stdin) fclose(f);

    if (!src) {
        fprintf(stderr, "Error: failed to read input.\n");
        return 1;
    }

    switch (mode) {
        case MODE_JSON:
            if (compact) compact_json(src, len);
            else beautify_json(src, len);
            break;
        case MODE_HTML:
            beautify_html(src, len);
            break;
        case MODE_CSS:
            beautify_css(src, len);
            break;
        case MODE_EXTRACT:
            extract_field(src, len, extract_key);
            break;
    }

    free(src);
    return 0;
}
