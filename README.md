# Portfolio Manager

**Repository:** [https://github.com/crissymoon/GitHub-Portfolio-Page-Builder](https://github.com/crissymoon/GitHub-Portfolio-Page-Builder)

A single-page portfolio system built with HTML, CSS, vanilla JavaScript, C, and Go. The C server runs cross-platform on Windows, macOS, and Linux with no runtime dependencies. The Go tool handles production builds. All content is driven by a single JSON data file. Fully responsive and compatible with older Safari versions.

---

## Features

- **JSON-driven content** -- every section (projects, links, skills, info, notice, education, socials) renders from a single data file
- **Paginated projects** -- displays 3 projects per page with previous/next controls, sortable by display order
- **Color theming** -- monochromatic color harmony with a customizable palette via the manager interface
- **Manager interface** -- a browser-based editor (`manage.html`) for modifying all content, colors, skills, projects, and links without touching code
- **Collapsible sections** -- every section in the manager collapses and expands with descriptions, plus an Expand All / Collapse All toggle
- **AI content generation** -- built-in OpenAI integration for generating notice text, bios, and project descriptions
- **Portfolio agent** -- a chat interface that uses AI to edit your portfolio via natural language, preview changes, and deploy
- **Social links** -- add GitHub, LinkedIn, Twitter, and other social media links displayed in the portfolio header
- **Support link** -- optional Buy Me a Coffee style donation link on the portfolio
- **Education section** -- togglable education and technical background summary on the portfolio
- **Resume PDF export** -- generates a one-page PDF resume from portfolio data with multi-CDN fallback loading
- **Deploy from manager** -- configure a GitHub Pages repo, check remote status, and deploy with one click from the browser
- **Go build tool** -- compiles all assets (CSS, JS, JSON) into self-contained HTML files for deployment
- **Multi-page output** -- produces identical copies across multiple URL paths so existing bookmarks and links continue to work
- **Built-in local server** -- a cross-platform C HTTP server with zero dependencies, compiles on Windows, macOS, and Linux
- **Port conflict resolution** -- if the server port is occupied, prompts to kill the process, choose a different port, or quit
- **One-click launchers** -- shell script (macOS/Linux) and batch file (Windows) that compile and start the server automatically
- **No external dependencies** -- zero frameworks, zero CDN links, zero runtime requirements beyond a C compiler
- **Responsive layout** -- works across desktop, tablet, and mobile with breakpoints at 768px and 480px
- **Older Safari support** -- uses webkit-prefixed flexbox properties for compatibility

---

## File Structure

```
.
├── index.html          # Portfolio page (links external CSS/JS)
├── styles.css          # All styles with CSS custom properties
├── scripts.js          # Vanilla JS: data loading, rendering, pagination
├── crissy-data.json    # All portfolio content and color configuration
├── manage.html         # Browser-based content editor with AI generation and agent
├── serve.c             # Cross-platform HTTP server (C, no dependencies)
├── build.go            # Go CLI that inlines everything into static HTML
├── deploy.conf         # Deploy target configuration (repo URL, custom domain)
├── push.sh             # Git push helper script
├── launch.sh           # macOS / Linux launcher (compiles and runs server)
├── launch.bat          # Windows launcher (compiles and runs server)
├── agent/              # Portfolio agent documentation
│   └── README.txt      # Agent usage and privacy notes
├── deploy/             # GitHub Pages deploy tool
│   ├── deploy.c        # Clone-based deploy with CNAME preservation (C)
│   └── README.md       # Documentation for the deploy tool
├── cmds/               # Agent tool-call utilities
│   ├── run.c           # Cross-platform command runner with approval gate
│   └── README.md       # Documentation for the command runner
├── img_convert/        # Image-to-base64 conversion utility
│   ├── convert.c       # Cross-platform image encoder (C, no dependencies)
│   └── README.md       # Documentation for the converter
└── build/              # Output directory (generated)
    ├── index.html
    ├── projects.html
    ├── links.html
    └── about.html
```

---

## Getting Started

### Prerequisites

- A C compiler for the local server (see below)
- [Go](https://go.dev/dl/) 1.18 or later (only needed for the production build tool)

Most systems already have a C compiler available. If not:

| OS | Install |
|---|---|
| macOS | `xcode-select --install` |
| Ubuntu / Debian | `sudo apt install gcc` |
| Fedora | `sudo dnf install gcc` |
| Windows | [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) or [MinGW-w64](https://www.mingw-w64.org/) |

### 1. Quick Start (Recommended)

Run the launcher script. It compiles the C server automatically and opens the manager in your browser.

**macOS / Linux:**

```sh
chmod +x launch.sh
./launch.sh
```

**Windows:**

```
launch.bat
```

This starts a local server on port 9090, opens `manage.html` in your default browser, and you can use the **View Portfolio** button to see the live site. To use a different port:

```sh
./launch.sh 3000
```

```
launch.bat 3000
```

### 2. Configure Your Data

Edit `crissy-data.json` directly or use the manager interface that opened in your browser.

The JSON structure:

```json
{
  "site": {
    "title": "Your Portfolio",
    "image": "data:image/png;base64,...",
    "imageBorder": true,
    "imageBorderColor": "#ffffff",
    "imageSize": "medium",
    "outputFiles": ["index.html", "projects.html", "links.html", "about.html"]
  },
  "notice": {
    "text": "Latest update message here.",
    "link": "https://example.com/",
    "updated": "2026-02-17"
  },
  "info": {
    "name": "Your Name",
    "email": "you@example.com",
    "bio": "Short bio.",
    "skills": ["HTML", "CSS", "JavaScript", "Go"]
  },
  "education": {
    "summary": "Your education and technical background.",
    "showOnPortfolio": true
  },
  "projects": [
    {
      "id": 1,
      "displayOrder": 1,
      "title": "Project Name",
      "description": "What it does.",
      "url": "https://example.com/",
      "tags": ["web", "tools"]
    }
  ],
  "links": [
    { "label": "Website", "url": "https://example.com/" }
  ],
  "socials": [
    { "platform": "GitHub", "url": "https://github.com/username" },
    { "platform": "LinkedIn", "url": "https://linkedin.com/in/username" }
  ],
  "supportLink": {
    "label": "Buy This Dev a Cup of Coffee",
    "url": "https://buymeacoffee.com/yourname"
  },
  "styles": {
    "primary": "#56008f",
    "secondary": "#f1eaf6",
    "accent": "#180029",
    "text": "#201825",
    "background": "#fdfcfd",
    "harmony": ["#56008f", "#9300f5", "#180029"],
    "bodyFont": "-apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif",
    "headingFont": "-apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif",
    "headingWeight": "700",
    "baseSize": "16px",
    "borderRadius": "0px"
  }
}
```

### 3. Use the Manager Interface

If the server is running (via the launcher), `manage.html` is already open. All sections are collapsible -- click a heading to expand or collapse it. Use the **Expand All** button to open everything at once. From the manager you can:

- Edit site title, notice text, and personal info
- Upload and configure a portfolio image (size, border, border color)
- Add and remove skills (click a skill tag to remove it)
- Add, edit, reorder, and remove projects (drag display order with up/down buttons)
- Add and remove links
- Add social media links (GitHub, LinkedIn, Twitter, etc.) shown in the portfolio header
- Set up a support / donation link (Buy Me a Coffee style)
- Write an education and technical background summary with toggle to show on portfolio
- Export a one-page PDF resume from your portfolio data
- Change all theme colors with color pickers
- Configure which output files the build tool generates
- Save, build, download JSON, or preview the portfolio
- Deploy directly to GitHub Pages from the Deploy section
- Chat with the Portfolio Agent to make edits using natural language
- Generate content with AI (see below)

### AI Content Generation

The manager includes built-in AI content generation for text fields (notice, bio, project descriptions). It calls the OpenAI API directly from the browser.

**Setup:**

1. Open `manage.html`
2. Under **AI Content Generation**, either paste your OpenAI API key or click the file picker to load it from a `.txt` file
3. Select the model (GPT-5.2 or GPT-5.1)

**Usage:**

Each text field that supports generation has a **Generate** button beside it. Click it to reveal a prompt box, type what you want, and click **Generate Content**. The AI output replaces the field value.

**Writing rules enforced automatically:**

- No emojis
- No contractions (all words written out fully)
- No em dashes or en dashes
- No spaced hyphens ( - )
- No exclamation marks
- No filler or marketing language
- Academic casual conversational tone
- Concise, direct output with no meta-commentary

These rules are enforced both in the system prompt sent to the API and in a post-processing pass that catches any remaining violations before the text is inserted.

### 4. Build for Production

Compile the build tool and run it:

```sh
go build -o portfolio-build build.go
./portfolio-build .
```

This reads `index.html`, `styles.css`, `scripts.js`, and `crissy-data.json`, inlines everything into a single self-contained HTML file, then writes identical copies to `build/` for each filename listed in `site.outputFiles`.

The output files contain no external references. The JS is patched at build time to use embedded data instead of XHR.

### 5. Deploy

There are two ways to deploy:

**Option A: One-click deploy from the manager**

1. In `manage.html`, expand the **Deploy to GitHub Pages** section
2. Enter your GitHub Pages repo URL (e.g., `https://github.com/user/user.github.io.git`)
3. Optionally enter a custom domain
4. Click **Save, Build + Deploy**

This saves your data, builds the portfolio, and pushes to GitHub Pages automatically. The deploy tool preserves any existing CNAME file in the repo.

**Option B: Manual deploy**

Copy the contents of `build/` to your hosting provider's public directory.

---

## Deploying with GitHub Pages

### Step 1: Create a Repository

```sh
git init
git add .
git commit -m "initial commit"
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git
git push -u origin main
```

### Step 2: Configure GitHub Pages

1. Go to your repository on GitHub
2. Navigate to **Settings** > **Pages**
3. Under **Source**, select **Deploy from a branch**
4. Select the **main** branch and set the folder to `/build` (or `/` if you copy build output to root)
5. Click **Save**

Your site will be live at `https://YOUR_USERNAME.github.io/YOUR_REPO/` within a few minutes.

### Step 3: Set Up a Custom Domain

1. In your repository, go to **Settings** > **Pages**
2. Under **Custom domain**, enter your domain (e.g., `yourdomain.com`)
3. Click **Save** -- GitHub will create a `CNAME` file in your repo

#### DNS Configuration

At your domain registrar or DNS provider, add the following records:

**For an apex domain** (e.g., `yourdomain.com`):

| Type | Name | Value |
|------|------|-------|
| A | @ | 185.199.108.153 |
| A | @ | 185.199.109.153 |
| A | @ | 185.199.110.153 |
| A | @ | 185.199.111.153 |

**For a subdomain** (e.g., `www.yourdomain.com`):

| Type | Name | Value |
|------|------|-------|
| CNAME | www | YOUR_USERNAME.github.io |

4. Wait for DNS propagation (can take up to 24-48 hours, usually much faster)
5. Back in GitHub Pages settings, check **Enforce HTTPS** once the certificate is provisioned

### Step 4: Automate Builds (Optional)

Create `.github/workflows/build.yml` to rebuild on every push:

```yaml
name: Build Portfolio

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - uses: actions/setup-go@v5
        with:
          go-version: '1.22'

      - name: Build
        run: |
          go build -o portfolio-build build.go
          ./portfolio-build .

      - name: Deploy
        uses: peaceiris/actions-gh-pages@v4
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./build
```

This builds the portfolio and deploys the `build/` directory to the `gh-pages` branch automatically.

---

## Agent Command Runner

The `cmds/` directory contains a cross-platform C tool called `run` that lets an AI agent execute terminal commands through a human approval gate. The agent sends a command, the operator sees exactly what will run, and nothing executes until the operator types `y`.

### Build

```sh
cd cmds
gcc -O2 -o run run.c
```

On macOS, if headers are not found:

```sh
cc -O2 -o run run.c --sysroot="$(xcrun --show-sdk-path)"
```

### Quick Example

```sh
./cmds/run ls -la
```

The tool prints the command for review, waits for approval, runs it if approved, and outputs a structured JSON result. It also supports `--json` mode for agent tool-call protocols (pipe `{"cmd": "..."}` on stdin) and `--log` for an append-only audit trail.

See [cmds/README.md](cmds/README.md) for full documentation including flags, exit codes, output format, integration patterns, and security notes.

---

## Image Converter

The `img_convert/` directory contains a cross-platform C tool that converts image files into base64 data URLs. The encoded output embeds directly in HTML, CSS, or JSON, which means the built portfolio pages are fully self-contained with no external image references. This works on GitHub Pages and any other static host.

### Build

```sh
cd img_convert
gcc -O2 -o convert convert.c
```

On macOS, if headers are not found:

```sh
cc -O2 -o convert convert.c --sysroot="$(xcrun --show-sdk-path)"
```

### Quick Example

```sh
./img_convert/convert avatar.png
```

Outputs a `data:image/png;base64,...` string. Paste it into the `site.image` field in `crissy-data.json`, or use `--json`, `--field`, `--css`, or `--html` flags for different output formats. Supports PNG, JPEG, GIF, SVG, WebP, ICO, BMP, TIFF, and AVIF.

See [img_convert/README.md](img_convert/README.md) for full documentation including output modes, workflow examples, and size guidelines.

---

## Deploy Tool

The `deploy/` directory contains a C tool that deploys the `build/` output to a GitHub Pages repository. It reads the repo URL and custom domain from `deploy.conf`, clones the remote repo into a temporary directory, replaces the content with build output, preserves the CNAME file if present, and force-pushes to the remote.

### Build

```sh
cd deploy
gcc -O2 -o deploy deploy.c
```

On macOS, if headers are not found:

```sh
cc -O2 -o deploy deploy.c --sysroot="$(xcrun --show-sdk-path)"
```

### Configuration

Create a `deploy.conf` file in the project root:

```
repo=https://github.com/YOUR_USERNAME/YOUR_REPO.git
domain=yourdomain.com
```

Or configure it from the manager's Deploy section, which saves this file automatically.

See [deploy/README.md](deploy/README.md) for full documentation.

---

## Portfolio Agent

The manager includes a built-in chat interface under the **Portfolio Agent** section. It uses the OpenAI API key stored in your browser's localStorage (never committed to the repo) to accept natural language instructions and edit your portfolio.

### Usage

1. Configure your API key in the AI Content Generation section
2. Expand the **Portfolio Agent** section
3. Type an instruction, for example:
   - "Change the site title to My New Portfolio"
   - "Add a project called Dashboard about analytics"
   - "Update the bio to mention backend experience"
   - "Change the primary color to dark blue"
4. The agent applies the changes, saves, builds, and opens a preview
5. It then asks whether you want to deploy the changes live

The API key is stored only in browser localStorage. It is sent directly from the browser to the OpenAI API and never passes through the local server or any file in this repository.

See [agent/README.txt](agent/README.txt) for more details.

---

## Resume PDF Export

The manager includes a one-page PDF resume exporter under the **Resume Export** section. It uses the jsPDF library (loaded from CDN with multi-source fallback) to generate a professional resume from your portfolio data.

The resume includes:

- Name and contact information (email, phone, portfolio URL)
- Social links
- Bio / summary
- Education and technical background
- Skills
- Top 3 projects (with a link to the full portfolio for more)
- Links
- Footer with the GitHub repo URL

Phone number is requested via prompt at export time and is not stored anywhere.

---

## Color Theming

Colors are defined in the `styles` section of `crissy-data.json`. The CSS uses custom properties (`--primary`, `--secondary`, etc.) that the JavaScript applies at runtime from the JSON data.

The default palette uses a **monochromatic** harmony. To change it:

1. Open `manage.html`
2. Use the color pickers under the **Colors** section
3. Save or download the updated JSON
4. Rebuild

---

## Adding Output Pages

To add or remove output filenames (so all URLs resolve to the same page), edit the `site.outputFiles` array in `crissy-data.json` or use the manager interface under **Output Files**. Then rebuild.

---

## Tech Stack

- **HTML5** -- semantic markup
- **CSS3** -- custom properties, flexbox with webkit prefixes
- **Vanilla JavaScript** -- ES5 compatible, no transpilation needed
- **C** -- cross-platform local HTTP server, compiles on Windows (MSVC, MinGW), macOS, and Linux with no external libraries
- **JSON** -- single data source
- **Go** -- build tool for asset inlining

---

## License

MIT
