/*
 * convert.c - Cross-platform image-to-base64 converter with auto-optimization
 *
 * Reads one or more image files, automatically optimizes them to
 * portfolio-appropriate sizes, and outputs base64-encoded data URLs
 * that can be embedded directly in HTML, CSS, or JSON.  The output
 * works on GitHub Pages and any other static host because the image
 * data lives inside the file itself -- no external references needed.
 *
 * Optimization uses platform-native tools with zero extra dependencies:
 *   macOS  -- sips (built in)
 *   Linux  -- magick / convert (ImageMagick) if available
 *   Windows -- PowerShell System.Drawing
 *
 * Build:
 *   gcc -O2 -o convert convert.c          (macOS / Linux)
 *   cl convert.c /Fe:convert.exe          (Windows MSVC)
 *   gcc -O2 -o convert.exe convert.c      (Windows MinGW)
 *
 * Usage:
 *   ./convert photo.png                    (auto-optimizes to 512px max)
 *   ./convert --max 256 avatar.png         (resize to 256px max)
 *   ./convert --no-optimize photo.png      (skip optimization, encode raw)
 *   ./convert --json photo.png logo.svg
 *   ./convert --field site.image avatar.jpg
 *   ./convert --css bg.png
 *   ./convert --html avatar.png
 *   ./convert --wrap 76 photo.png
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ---- Platform ---- */

#ifdef _WIN32
  #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
  #endif
  #include <io.h>
  #include <fcntl.h>
  #include <process.h>
  #include <direct.h>
  #define PATH_SEP '\\'
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #define PATH_SEP '/'
#endif

/* ---- Base64 encoding ---- */

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Encode src (len bytes) into a null-terminated base64 string.
 * Caller must free the returned pointer.
 */
static char *base64_encode(const unsigned char *src, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = (char *)malloc(out_len + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        unsigned int a = i < len ? src[i++] : 0;
        unsigned int b = i < len ? src[i++] : 0;
        unsigned int c = i < len ? src[i++] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;

        out[j++] = B64[(triple >> 18) & 0x3F];
        out[j++] = B64[(triple >> 12) & 0x3F];
        out[j++] = B64[(triple >>  6) & 0x3F];
        out[j++] = B64[(triple      ) & 0x3F];
    }

    /* Pad */
    size_t mod = len % 3;
    if (mod == 1) {
        out[j - 1] = '=';
        out[j - 2] = '=';
    } else if (mod == 2) {
        out[j - 1] = '=';
    }

    out[j] = '\0';
    return out;
}

/* ---- MIME type detection ---- */

typedef struct {
    const char *ext;
    const char *mime;
} mime_entry;

static const mime_entry MIME_TABLE[] = {
    { ".png",  "image/png"      },
    { ".jpg",  "image/jpeg"     },
    { ".jpeg", "image/jpeg"     },
    { ".gif",  "image/gif"      },
    { ".svg",  "image/svg+xml"  },
    { ".webp", "image/webp"     },
    { ".ico",  "image/x-icon"   },
    { ".bmp",  "image/bmp"      },
    { ".tiff", "image/tiff"     },
    { ".tif",  "image/tiff"     },
    { ".avif", "image/avif"     },
    { NULL,    NULL             }
};

static const char *detect_mime(const char *filename) {
    const char *dot = NULL;
    const char *p = filename;
    while (*p) {
        if (*p == '.') dot = p;
        p++;
    }
    if (!dot) return "application/octet-stream";

    /* Case-insensitive extension match */
    char ext_lower[32];
    size_t ext_len = strlen(dot);
    if (ext_len >= sizeof(ext_lower)) ext_len = sizeof(ext_lower) - 1;
    for (size_t i = 0; i < ext_len; i++) {
        char c = dot[i];
        ext_lower[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    ext_lower[ext_len] = '\0';

    for (int i = 0; MIME_TABLE[i].ext; i++) {
        if (strcmp(ext_lower, MIME_TABLE[i].ext) == 0) {
            return MIME_TABLE[i].mime;
        }
    }
    return "application/octet-stream";
}

/* ---- File reading ---- */

static unsigned char *read_file(const char *path, size_t *out_len) {
#ifdef _WIN32
    FILE *fp = fopen(path, "rb");
#else
    FILE *fp = fopen(path, "rb");
#endif
    if (!fp) {
        fprintf(stderr, "convert: cannot open %s\n", path);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0) {
        fprintf(stderr, "convert: %s is empty or unreadable\n", path);
        fclose(fp);
        return NULL;
    }

    unsigned char *buf = (unsigned char *)malloc((size_t)fsize);
    if (!buf) {
        fprintf(stderr, "convert: out of memory reading %s\n", path);
        fclose(fp);
        return NULL;
    }

    size_t nread = fread(buf, 1, (size_t)fsize, fp);
    fclose(fp);

    if (nread != (size_t)fsize) {
        fprintf(stderr, "convert: short read on %s\n", path);
        free(buf);
        return NULL;
    }

    *out_len = (size_t)fsize;
    return buf;
}

/* ---- Temp file path ---- */

static void make_temp_path(char *buf, size_t buf_len, const char *original) {
    const char *base = original;
    const char *p = original;
    while (*p) {
        if (*p == '/' || *p == '\\') base = p + 1;
        p++;
    }

    /* Find extension */
    const char *dot = NULL;
    p = base;
    while (*p) {
        if (*p == '.') dot = p;
        p++;
    }

#ifdef _WIN32
    const char *tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = ".";
    if (dot) {
        snprintf(buf, buf_len, "%s\\_cvt_opt_%s", tmp, base);
    } else {
        snprintf(buf, buf_len, "%s\\_cvt_opt_%s.png", tmp, base);
    }
#else
    if (dot) {
        snprintf(buf, buf_len, "/tmp/_cvt_opt_%s", base);
    } else {
        snprintf(buf, buf_len, "/tmp/_cvt_opt_%s.png", base);
    }
#endif
}

/* ---- Image optimization ---- */

/*
 * Returns 1 if the extension is a raster format that can be resized.
 * SVG and ICO are excluded -- SVG is already small and vector,
 * ICO has special multi-resolution structure.
 */
static int is_optimizable(const char *filename) {
    const char *dot = NULL;
    const char *p = filename;
    while (*p) {
        if (*p == '.') dot = p;
        p++;
    }
    if (!dot) return 0;

    char ext[32];
    size_t len = strlen(dot);
    if (len >= sizeof(ext)) len = sizeof(ext) - 1;
    for (size_t i = 0; i < len; i++) {
        char c = dot[i];
        ext[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    ext[len] = '\0';

    /* Raster formats we can resize */
    if (strcmp(ext, ".png") == 0)  return 1;
    if (strcmp(ext, ".jpg") == 0)  return 1;
    if (strcmp(ext, ".jpeg") == 0) return 1;
    if (strcmp(ext, ".gif") == 0)  return 1;
    if (strcmp(ext, ".webp") == 0) return 1;
    if (strcmp(ext, ".bmp") == 0)  return 1;
    if (strcmp(ext, ".tiff") == 0) return 1;
    if (strcmp(ext, ".tif") == 0)  return 1;
    if (strcmp(ext, ".avif") == 0) return 1;

    return 0;
}

/*
 * Try to get image dimensions from platform tools.
 * Returns 1 on success, 0 on failure.
 */
static int get_image_dimensions(const char *path, int *w, int *h) {
    char cmd[2048];
    FILE *pp;

#ifdef __APPLE__
    /* sips is always available on macOS */
    snprintf(cmd, sizeof(cmd),
        "sips -g pixelWidth -g pixelHeight '%s' 2>/dev/null", path);
    pp = popen(cmd, "r");
    if (!pp) return 0;

    char line[512];
    *w = 0;
    *h = 0;
    while (fgets(line, sizeof(line), pp)) {
        if (strstr(line, "pixelWidth"))  sscanf(line, "%*s %d", w);
        if (strstr(line, "pixelHeight")) sscanf(line, "%*s %d", h);
    }
    pclose(pp);
    return (*w > 0 && *h > 0);

#elif defined(_WIN32)
    /* PowerShell with System.Drawing */
    snprintf(cmd, sizeof(cmd),
        "powershell -NoProfile -Command \""
        "Add-Type -Assembly System.Drawing;"
        "$img=[System.Drawing.Image]::FromFile('%s');"
        "Write-Host $img.Width $img.Height;"
        "$img.Dispose()\"", path);
    pp = _popen(cmd, "r");
    if (!pp) return 0;

    char line[256];
    if (fgets(line, sizeof(line), pp)) {
        sscanf(line, "%d %d", w, h);
    }
    _pclose(pp);
    return (*w > 0 && *h > 0);

#else
    /* Linux: try identify (ImageMagick), then file command */
    snprintf(cmd, sizeof(cmd),
        "identify -format '%%w %%h' '%s' 2>/dev/null"
        " || magick identify -format '%%w %%h' '%s' 2>/dev/null",
        path, path);
    pp = popen(cmd, "r");
    if (!pp) return 0;

    char line[256];
    *w = 0;
    *h = 0;
    if (fgets(line, sizeof(line), pp)) {
        sscanf(line, "%d %d", w, h);
    }
    pclose(pp);
    return (*w > 0 && *h > 0);
#endif
}

/*
 * Optimize an image: resize to fit within max_dim x max_dim
 * while preserving aspect ratio. Saves result to tmp_path.
 * Returns 1 on success, 0 on failure (caller should use original).
 */
static int optimize_image(const char *src, const char *tmp_path,
                          int max_dim, int quiet) {
    int w = 0, h = 0;

    if (!get_image_dimensions(src, &w, &h)) {
        if (!quiet) {
            fprintf(stderr, "  [optimize] cannot read dimensions, skipping optimization\n");
        }
        return 0;
    }

    /* Already small enough? */
    if (w <= max_dim && h <= max_dim) {
        if (!quiet) {
            fprintf(stderr, "  [optimize] %dx%d already within %dpx, no resize needed\n",
                    w, h, max_dim);
        }
        return 0;
    }

    if (!quiet) {
        fprintf(stderr, "  [optimize] %dx%d -> resizing to fit %dpx ...\n",
                w, h, max_dim);
    }

    char cmd[4096];
    int ret;

#ifdef __APPLE__
    /* Copy to temp first so we don't modify the original */
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", src, tmp_path);
    ret = system(cmd);
    if (ret != 0) return 0;

    /* sips resampleHeightWidthMax keeps aspect ratio */
    snprintf(cmd, sizeof(cmd),
        "sips --resampleHeightWidthMax %d '%s' >/dev/null 2>&1",
        max_dim, tmp_path);
    ret = system(cmd);
    if (ret != 0) {
        remove(tmp_path);
        return 0;
    }

#elif defined(_WIN32)
    /* PowerShell System.Drawing resize */
    snprintf(cmd, sizeof(cmd),
        "powershell -NoProfile -Command \""
        "Add-Type -Assembly System.Drawing;"
        "$src=[System.Drawing.Image]::FromFile('%s');"
        "$maxd=%d;"
        "$r=[Math]::Min($maxd/$src.Width,$maxd/$src.Height);"
        "if($r -ge 1){$src.Dispose();exit 1}"
        "$nw=[int]($src.Width*$r);"
        "$nh=[int]($src.Height*$r);"
        "$dst=New-Object System.Drawing.Bitmap($nw,$nh);"
        "$g=[System.Drawing.Graphics]::FromImage($dst);"
        "$g.InterpolationMode='HighQualityBicubic';"
        "$g.DrawImage($src,0,0,$nw,$nh);"
        "$dst.Save('%s');"
        "$g.Dispose();$dst.Dispose();$src.Dispose()\"",
        src, max_dim, tmp_path);
    ret = system(cmd);
    if (ret != 0) {
        remove(tmp_path);
        return 0;
    }

#else
    /* Linux: try magick (ImageMagick 7), then convert (ImageMagick 6) */
    snprintf(cmd, sizeof(cmd),
        "magick '%s' -resize '%dx%d>' '%s' 2>/dev/null"
        " || convert '%s' -resize '%dx%d>' '%s' 2>/dev/null",
        src, max_dim, max_dim, tmp_path,
        src, max_dim, max_dim, tmp_path);
    ret = system(cmd);
    if (ret != 0) {
        remove(tmp_path);
        return 0;
    }
#endif

    /* Verify the output file exists and has content */
    FILE *fp = fopen(tmp_path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fclose(fp);
    if (sz <= 0) {
        remove(tmp_path);
        return 0;
    }

    if (!quiet) {
        fprintf(stderr, "  [optimize] optimized file: %ld bytes\n", sz);
    }
    return 1;
}

/* ---- JSON string escaping (for the data URL value) ---- */

static void print_json_string(const char *s) {
    putchar('"');
    while (*s) {
        switch (*s) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\n': fputs("\\n", stdout);  break;
            case '\r': fputs("\\r", stdout);  break;
            case '\t': fputs("\\t", stdout);  break;
            default:   putchar(*s);           break;
        }
        s++;
    }
    putchar('"');
}

/* ---- Output with optional line wrapping ---- */

static void print_wrapped(const char *s, int wrap) {
    if (wrap <= 0) {
        fputs(s, stdout);
        return;
    }
    size_t len = strlen(s);
    size_t pos = 0;
    while (pos < len) {
        size_t chunk = (size_t)wrap;
        if (pos + chunk > len) chunk = len - pos;
        fwrite(s + pos, 1, chunk, stdout);
        pos += chunk;
        if (pos < len) putchar('\n');
    }
}

/* ---- Filename extraction ---- */

static const char *basename_of(const char *path) {
    const char *last = path;
    while (*path) {
        if (*path == '/' || *path == '\\') last = path + 1;
        path++;
    }
    return last;
}

/* ---- Usage ---- */

static void print_usage(void) {
    fprintf(stderr,
        "Usage: convert [flags] <image> [image2 ...]\n"
        "\n"
        "Converts image files to base64 data URLs for embedding in\n"
        "HTML, CSS, or JSON. Automatically optimizes large images to\n"
        "portfolio-appropriate sizes before encoding.\n"
        "\n"
        "Optimization (on by default for raster images):\n"
        "  --max N         Max pixel dimension (default 512). Images\n"
        "                  larger than NxN are resized to fit, keeping\n"
        "                  aspect ratio. SVG and ICO are never resized.\n"
        "  --no-optimize   Skip optimization, encode the raw file as-is\n"
        "\n"
        "Output modes (default is raw data URL):\n"
        "  --json          Wrap each output in a JSON object\n"
        "  --field KEY     Output as a JSON key:value pair\n"
        "  --css           Output as a CSS url() value\n"
        "  --html          Output as an <img> tag\n"
        "\n"
        "Options:\n"
        "  --wrap N        Line-wrap the base64 at N characters (0 = no wrap)\n"
        "  --quiet         Suppress the file info line on stderr\n"
        "  --help          Print this message\n"
        "\n"
        "Optimization uses platform tools with zero extra dependencies:\n"
        "  macOS   sips (built in to every Mac)\n"
        "  Linux   magick or convert (ImageMagick)\n"
        "  Windows PowerShell System.Drawing\n"
        "\n"
        "Examples:\n"
        "  ./convert photo.png                  (auto-optimize to 512px)\n"
        "  ./convert --max 256 avatar.png       (optimize to 256px max)\n"
        "  ./convert --max 800 screenshot.png   (optimize to 800px max)\n"
        "  ./convert --no-optimize photo.png    (skip optimization)\n"
        "  ./convert --json avatar.jpg logo.svg\n"
        "  ./convert --field site.image avatar.png\n"
        "  ./convert --css background.webp\n"
        "  ./convert --html photo.png\n"
        "\n"
        "Supported formats: PNG, JPEG, GIF, SVG, WebP, ICO, BMP, TIFF, AVIF\n"
    );
}

/* ---- Output modes ---- */

enum output_mode {
    MODE_RAW,     /* just the data URL */
    MODE_JSON,    /* {"file":"...", "dataUrl":"..."} */
    MODE_FIELD,   /* "key": "data:..." */
    MODE_CSS,     /* url(data:...) */
    MODE_HTML     /* <img src="data:..." alt="..."> */
};

/* ---- Main ---- */

int main(int argc, char **argv) {
    enum output_mode mode = MODE_RAW;
    const char *field_key = NULL;
    int wrap = 0;
    int quiet = 0;
    int do_optimize = 1;       /* on by default */
    int max_dim = 512;         /* default max pixel dimension */
    int file_count = 0;
    char *files[256];

#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
    /* Actually we want text mode for the output, but binary for reads.
     * Reads use fopen("rb") directly.  Reset stdout to text. */
    _setmode(_fileno(stdout), _O_TEXT);
#endif

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            mode = MODE_JSON;
        } else if (strcmp(argv[i], "--field") == 0 && i + 1 < argc) {
            mode = MODE_FIELD;
            field_key = argv[++i];
        } else if (strcmp(argv[i], "--css") == 0) {
            mode = MODE_CSS;
        } else if (strcmp(argv[i], "--html") == 0) {
            mode = MODE_HTML;
        } else if (strcmp(argv[i], "--wrap") == 0 && i + 1 < argc) {
            wrap = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc) {
            max_dim = atoi(argv[++i]);
            if (max_dim < 16) max_dim = 16;  /* sanity floor */
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            do_optimize = 0;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else if (argv[i][0] == '-' && argv[i][1] == '-') {
            fprintf(stderr, "convert: unknown flag %s\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) {
                files[file_count++] = argv[i];
            }
        }
    }

    if (file_count == 0) {
        print_usage();
        return 1;
    }

    int first_json = 1;
    if (mode == MODE_JSON && file_count > 1) {
        printf("[\n");
    }

    for (int f = 0; f < file_count; f++) {
        const char *path = files[f];
        const char *read_path = path;  /* may point to optimized temp */
        char tmp_path[2048];
        int used_tmp = 0;
        size_t fsize = 0;

        /* Auto-optimize if enabled and format supports it */
        if (do_optimize && is_optimizable(path)) {
            make_temp_path(tmp_path, sizeof(tmp_path), path);
            if (optimize_image(path, tmp_path, max_dim, quiet)) {
                read_path = tmp_path;
                used_tmp = 1;
            }
        }

        unsigned char *data = read_file(read_path, &fsize);

        /* Clean up temp file immediately after reading */
        if (used_tmp) {
            remove(tmp_path);
        }

        if (!data) continue;

        const char *mime = detect_mime(path);
        char *b64 = base64_encode(data, fsize);
        free(data);

        if (!b64) {
            fprintf(stderr, "convert: encoding failed for %s\n", path);
            continue;
        }

        /* Build the data URL */
        size_t url_len = strlen("data:") + strlen(mime) + strlen(";base64,") + strlen(b64) + 1;
        char *data_url = (char *)malloc(url_len);
        if (!data_url) {
            free(b64);
            fprintf(stderr, "convert: out of memory\n");
            continue;
        }
        snprintf(data_url, url_len, "data:%s;base64,%s", mime, b64);
        free(b64);

        /* Info line on stderr */
        if (!quiet) {
            fprintf(stderr, "%s  (%s, %zu bytes, %zu chars base64)\n",
                    basename_of(path), mime, fsize, strlen(data_url));
        }

        /* Output */
        switch (mode) {
            case MODE_RAW:
                print_wrapped(data_url, wrap);
                printf("\n");
                break;

            case MODE_JSON:
                if (file_count > 1) {
                    if (!first_json) printf(",\n");
                    printf("  {\"file\": ");
                    print_json_string(basename_of(path));
                    printf(", \"mime\": ");
                    print_json_string(mime);
                    printf(", \"size\": %zu, \"dataUrl\": ", fsize);
                    print_json_string(data_url);
                    printf("}");
                } else {
                    printf("{\"file\": ");
                    print_json_string(basename_of(path));
                    printf(", \"mime\": ");
                    print_json_string(mime);
                    printf(", \"size\": %zu, \"dataUrl\": ", fsize);
                    print_json_string(data_url);
                    printf("}\n");
                }
                first_json = 0;
                break;

            case MODE_FIELD:
                printf("\"%s\": ", field_key ? field_key : "image");
                print_json_string(data_url);
                printf("\n");
                break;

            case MODE_CSS:
                printf("url(%s)", data_url);
                printf("\n");
                break;

            case MODE_HTML:
                printf("<img src=\"%s\" alt=\"%s\">\n",
                       data_url, basename_of(path));
                break;
        }

        free(data_url);
    }

    if (mode == MODE_JSON && file_count > 1) {
        printf("\n]\n");
    }

    return 0;
}
