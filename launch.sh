#!/bin/sh
# launch.sh - macOS and Linux launcher for the portfolio server
# This script compiles serve.c and starts the server.
# No dependencies required beyond a C compiler (cc/gcc/clang).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

PORT="${1:-9090}"
BINARY="./serve"

# Find a C compiler
CC=""
if command -v cc >/dev/null 2>&1; then
    CC="cc"
elif command -v gcc >/dev/null 2>&1; then
    CC="gcc"
elif command -v clang >/dev/null 2>&1; then
    CC="clang"
fi

if [ -z "$CC" ]; then
    echo ""
    echo "No C compiler found."
    echo ""
    echo "Install one with:"
    echo "  macOS:  xcode-select --install"
    echo "  Ubuntu: sudo apt install gcc"
    echo "  Fedora: sudo dnf install gcc"
    echo ""
    exit 1
fi

# Compile if binary is missing or source is newer
if [ ! -f "$BINARY" ] || [ "serve.c" -nt "$BINARY" ]; then
    echo "Compiling serve.c ..."
    # On macOS, pass the SDK sysroot if available
    SYSROOT_FLAG=""
    if command -v xcrun >/dev/null 2>&1; then
        SDK_PATH="$(xcrun --show-sdk-path 2>/dev/null)"
        if [ -n "$SDK_PATH" ]; then
            SYSROOT_FLAG="--sysroot=$SDK_PATH"
        fi
    fi
    $CC -O2 -o "$BINARY" serve.c $SYSROOT_FLAG
    echo "Done."
fi

echo ""
echo "Starting portfolio server on port $PORT ..."
echo ""
exec "$BINARY" "$PORT"
