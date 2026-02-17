/*
 * serve.c - Cross-platform static file HTTP server
 * Compiles and runs on Windows, macOS, and Linux with no dependencies.
 * Serves the current directory on a configurable port and opens the browser.
 *
 * Build:
 *   gcc -o serve serve.c        (macOS / Linux)
 *   cl serve.c /Fe:serve.exe    (Windows MSVC)
 *   gcc -o serve.exe serve.c -lws2_32  (Windows MinGW)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

/* ---- Platform Abstractions ---- */

#ifdef _WIN32
  #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSESOCKET closesocket
  #define INVALID_SOCK INVALID_SOCKET
  #define SOCKERR SOCKET_ERROR
  static int platform_init(void) {
      WSADATA wsa;
      return WSAStartup(MAKEWORD(2, 2), &wsa);
  }
  static void platform_cleanup(void) { WSACleanup(); }
  static void open_browser(int port) {
      char cmd[256];
      snprintf(cmd, sizeof(cmd), "start http://localhost:%d/manage.html", port);
      system(cmd);
  }
  #define PATH_SEP '\\'
  #define S_ISDIR(m) (((m) & _S_IFDIR) != 0)
  #define S_ISREG(m) (((m) & _S_IFREG) != 0)
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <errno.h>
  typedef int sock_t;
  #define CLOSESOCKET close
  #define INVALID_SOCK (-1)
  #define SOCKERR (-1)
  static int platform_init(void) { return 0; }
  static void platform_cleanup(void) {}
  static void open_browser(int port) {
      char cmd[256];
      #ifdef __APPLE__
        snprintf(cmd, sizeof(cmd), "open http://localhost:%d/manage.html", port);
      #else
        snprintf(cmd, sizeof(cmd), "xdg-open http://localhost:%d/manage.html 2>/dev/null || "
                                   "sensible-browser http://localhost:%d/manage.html 2>/dev/null || "
                                   "echo 'Open http://localhost:%d/manage.html in your browser'",
                                   port, port, port);
      #endif
      system(cmd);
  }
  #define PATH_SEP '/'
#endif

/* ---- Globals ---- */

static volatile int running = 1;
static sock_t server_sock = INVALID_SOCK;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
    if (server_sock != INVALID_SOCK) {
        CLOSESOCKET(server_sock);
        server_sock = INVALID_SOCK;
    }
}

/* ---- MIME Types ---- */

typedef struct {
    const char *ext;
    const char *mime;
} mime_entry;

static const mime_entry mime_table[] = {
    { ".html", "text/html; charset=utf-8" },
    { ".htm",  "text/html; charset=utf-8" },
    { ".css",  "text/css; charset=utf-8" },
    { ".js",   "application/javascript; charset=utf-8" },
    { ".json", "application/json; charset=utf-8" },
    { ".png",  "image/png" },
    { ".jpg",  "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".gif",  "image/gif" },
    { ".svg",  "image/svg+xml" },
    { ".ico",  "image/x-icon" },
    { ".txt",  "text/plain; charset=utf-8" },
    { ".md",   "text/plain; charset=utf-8" },
    { ".woff", "font/woff" },
    { ".woff2","font/woff2" },
    { ".ttf",  "font/ttf" },
    { ".xml",  "application/xml" },
    { NULL,    NULL }
};

static const char *get_mime(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot) {
        for (int i = 0; mime_table[i].ext; i++) {
            if (strcmp(dot, mime_table[i].ext) == 0) {
                return mime_table[i].mime;
            }
        }
    }
    return "application/octet-stream";
}

/* ---- Path Safety ---- */

static int path_is_safe(const char *path) {
    /* Block directory traversal */
    if (strstr(path, "..")) return 0;
    if (path[0] == '/') path++;
    if (path[0] == '\\') path++;
    /* Block absolute Windows paths */
    if (strlen(path) >= 2 && path[1] == ':') return 0;
    return 1;
}

/* ---- Send Helpers ---- */

static void send_response(sock_t client, int status, const char *status_text,
                          const char *content_type, const char *body, long body_len) {
    char header[1024];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status, status_text, content_type, body_len);
    send(client, header, hlen, 0);
    if (body && body_len > 0) {
        long sent = 0;
        while (sent < body_len) {
            long chunk = body_len - sent;
            if (chunk > 8192) chunk = 8192;
            int n = send(client, body + sent, (int)chunk, 0);
            if (n <= 0) break;
            sent += n;
        }
    }
}

static void send_error(sock_t client, int status, const char *text) {
    char body[256];
    int blen = snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1></body></html>", status, text);
    send_response(client, status, text, "text/html; charset=utf-8", body, blen);
}

static void send_file(sock_t client, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        send_error(client, 404, "Not Found");
        return;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(fsize);
    if (!buf) {
        fclose(f);
        send_error(client, 500, "Internal Server Error");
        return;
    }
    fread(buf, 1, fsize, f);
    fclose(f);

    const char *mime = get_mime(filepath);
    send_response(client, 200, "OK", mime, buf, fsize);
    free(buf);
}

/* ---- Request Handler ---- */

static void handle_request(sock_t client) {
    char buf[4096];
    int n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    /* Parse request line */
    char method[16] = {0};
    char raw_path[1024] = {0};
    sscanf(buf, "%15s %1023s", method, raw_path);

    /* Only handle GET */
    if (strcmp(method, "GET") != 0) {
        send_error(client, 405, "Method Not Allowed");
        return;
    }

    /* Strip query string */
    char *qmark = strchr(raw_path, '?');
    if (qmark) *qmark = '\0';

    /* URL decode (basic: handle %XX) */
    char path[1024];
    {
        int pi = 0;
        for (int i = 0; raw_path[i] && pi < (int)sizeof(path) - 1; i++) {
            if (raw_path[i] == '%' && raw_path[i+1] && raw_path[i+2]) {
                char hex[3] = { raw_path[i+1], raw_path[i+2], 0 };
                path[pi++] = (char)strtol(hex, NULL, 16);
                i += 2;
            } else {
                path[pi++] = raw_path[i];
            }
        }
        path[pi] = '\0';
    }

    /* Safety check */
    if (!path_is_safe(path)) {
        send_error(client, 403, "Forbidden");
        return;
    }

    /* Build local file path */
    char filepath[2048];
    const char *rel = path;
    if (rel[0] == '/') rel++;

    if (rel[0] == '\0') {
        /* Root request -> serve manage.html */
        snprintf(filepath, sizeof(filepath), "manage.html");
    } else {
        snprintf(filepath, sizeof(filepath), "%s", rel);
    }

    /* Convert slashes on Windows */
    #ifdef _WIN32
    for (int i = 0; filepath[i]; i++) {
        if (filepath[i] == '/') filepath[i] = '\\';
    }
    #endif

    /* Check if path is a directory, try index.html */
    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        char idx[2048];
        snprintf(idx, sizeof(idx), "%s%cindex.html", filepath, PATH_SEP);
        if (stat(idx, &st) == 0 && S_ISREG(st.st_mode)) {
            send_file(client, idx);
        } else {
            send_error(client, 403, "Forbidden");
        }
        return;
    }

    if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
        send_file(client, filepath);
    } else {
        send_error(client, 404, "Not Found");
    }
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    int port = 9090;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }

    signal(SIGINT, handle_signal);
    #ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
    #endif

    if (platform_init() != 0) {
        fprintf(stderr, "Failed to initialize networking.\n");
        return 1;
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCK) {
        fprintf(stderr, "Failed to create socket.\n");
        platform_cleanup();
        return 1;
    }

    /* Allow port reuse */
    int opt = 1;
    #ifdef _WIN32
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    #else
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKERR) {
        fprintf(stderr, "Failed to bind to port %d. It may be in use.\n", port);
        CLOSESOCKET(server_sock);
        platform_cleanup();
        return 1;
    }

    if (listen(server_sock, 16) == SOCKERR) {
        fprintf(stderr, "Failed to listen.\n");
        CLOSESOCKET(server_sock);
        platform_cleanup();
        return 1;
    }

    printf("Portfolio server running at http://localhost:%d/\n", port);
    printf("Manager:   http://localhost:%d/manage.html\n", port);
    printf("Portfolio: http://localhost:%d/index.html\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    open_browser(port);

    while (running) {
        struct sockaddr_in client_addr;
        #ifdef _WIN32
        int client_len = sizeof(client_addr);
        #else
        socklen_t client_len = sizeof(client_addr);
        #endif

        sock_t client = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client == INVALID_SOCK) {
            if (!running) break;
            continue;
        }

        handle_request(client);
        CLOSESOCKET(client);
    }

    printf("\nServer stopped.\n");
    if (server_sock != INVALID_SOCK) {
        CLOSESOCKET(server_sock);
    }
    platform_cleanup();
    return 0;
}
