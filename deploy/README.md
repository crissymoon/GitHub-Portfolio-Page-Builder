# deploy/

Cross-platform GitHub Pages deploy tool. Pushes the built portfolio files from `build/` to a separate GitHub Pages repository.

## Build

**macOS**
```
cc -O2 -o deploy deploy.c --sysroot="$(xcrun --show-sdk-path)"
```

**Linux**
```
gcc -O2 -o deploy deploy.c
```

**Windows (MSVC)**
```
cl deploy.c /Fe:deploy.exe
```

**Windows (MinGW)**
```
gcc -O2 -o deploy.exe deploy.c
```

## Usage

```
./deploy <repo-url>          # Deploy and save the repo URL
./deploy                     # Deploy using the saved repo URL
./deploy --set <repo-url>    # Save URL to deploy.conf without deploying
./deploy --config             # Print current saved config
```

## How It Works

1. Reads the GitHub Pages repo URL from `deploy.conf` (or accepts it as an argument)
2. Locates the `build/` directory containing the generated portfolio HTML files
3. Creates a temporary staging git repo
4. Copies all build files into the staging repo
5. Adds a `.nojekyll` file (tells GitHub Pages to skip Jekyll processing)
6. Commits and force-pushes to the target repo's `main` branch
7. Cleans up the staging directory
8. Prints the expected GitHub Pages URL

## Config File

The repo URL is saved in `deploy.conf` in the project root:

```
# deploy.conf - GitHub Pages deploy target
repo=https://github.com/username/username.github.io.git
```

This file is safe to commit since the repo is public.

## Requirements

- `git` must be installed and available in PATH
- Git credentials must be configured (SSH key or credential helper)
- The target GitHub repo must exist (create it on GitHub first)
- GitHub Pages must be enabled on the repo (Settings > Pages > Source: main branch)

## Workflow

1. Edit your portfolio in the manager
2. Click Save, then Build
3. Click Deploy (or run `./deploy` from the terminal)
4. Your site goes live at `https://username.github.io/repo-name/`
