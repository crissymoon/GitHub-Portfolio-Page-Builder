#!/bin/sh
# push.sh - Push portfolio to GitHub repository
# Commit message uses the timestamp and portfolio title from crissy-data.json

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

REPO="https://github.com/crissymoon/GitHub-Portfolio-Page-Builder.git"

# Extract the portfolio title from crissy-data.json
TITLE=""
if command -v python3 >/dev/null 2>&1; then
    TITLE=$(python3 -c "import json; print(json.load(open('crissy-data.json'))['site']['title'])" 2>/dev/null || true)
elif command -v python >/dev/null 2>&1; then
    TITLE=$(python -c "import json; print(json.load(open('crissy-data.json'))['site']['title'])" 2>/dev/null || true)
fi

if [ -z "$TITLE" ]; then
    # Fallback: grep it out
    TITLE=$(grep -o '"title"[[:space:]]*:[[:space:]]*"[^"]*"' crissy-data.json | head -1 | sed 's/.*: *"//;s/"//')
fi

if [ -z "$TITLE" ]; then
    TITLE="Portfolio"
fi

TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")
COMMIT_MSG="$TIMESTAMP $TITLE"

# Initialize git if needed
if [ ! -d ".git" ]; then
    git init
    git remote add origin "$REPO"
fi

# Verify remote matches
CURRENT_REMOTE=$(git remote get-url origin 2>/dev/null || true)
if [ "$CURRENT_REMOTE" != "$REPO" ]; then
    git remote set-url origin "$REPO" 2>/dev/null || git remote add origin "$REPO"
fi

# Ensure we are on main branch
git checkout -B main 2>/dev/null

# Stage all files, excluding build artifacts and binaries
cat > .gitignore << 'GITIGNORE'
.DS_Store
serve
portfolio-build
*.exe
*.o
*.obj
GITIGNORE

git add -A
git commit -m "$COMMIT_MSG"
git push -u origin main

echo ""
echo "Pushed: $COMMIT_MSG"
