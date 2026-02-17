# img_convert -- Image to Base64 Converter

A cross-platform C tool that converts image files into base64 data URLs for embedding directly in HTML, CSS, or JSON. Large images are automatically optimized to portfolio-appropriate sizes before encoding, using platform-native tools with zero extra dependencies. The encoded output is fully self-contained, so it works on GitHub Pages, any static host, or when opened as a local file -- no image hosting, no external URLs, no CORS issues.

---

## Does This Work on GitHub Pages?

Yes. Base64 data URLs embed the entire image payload inside the HTML/CSS/JSON source. GitHub Pages serves these files as-is. The browser decodes the base64 string and renders the image without making any additional HTTP requests. This means:

- No separate image files to upload or manage
- No broken images from wrong paths or missing assets
- The Go build tool already inlines everything into self-contained HTML
- A single HTML file contains the full page including all images
- Works in every modern browser and every version of Safari the project supports

The tradeoff is file size. Base64 encoding adds roughly 33% overhead compared to the raw binary. For portfolio images (avatars, project screenshots), this is negligible. The tool automatically resizes large images to 512px max before encoding, so you do not need to manually resize anything.

---

## Automatic Optimization

The converter automatically optimizes raster images before encoding. When you run `./convert photo.png`, the tool:

1. Reads the image dimensions using platform-native tools
2. If either dimension exceeds 512px (configurable with `--max`), resizes the image to fit within that box while preserving aspect ratio
3. Encodes the optimized version as base64
4. Cleans up any temporary files

This happens transparently. A 4000x3000 photo becomes a 512x384 image before encoding, cutting the base64 output by 95% or more.

### Platform tools used

| Platform | Tool | Notes |
|----------|------|-------|
| macOS | `sips` | Built in to every Mac, no install needed |
| Linux | `magick` or `convert` | ImageMagick, available in most distros |
| Windows | PowerShell `System.Drawing` | Built in, no install needed |

### Controlling optimization

```sh
./convert photo.png                    # auto-optimize to 512px max
./convert --max 256 avatar.png         # resize to 256px max dimension
./convert --max 800 screenshot.png     # resize to 800px max dimension
./convert --no-optimize photo.png      # skip optimization, encode raw file
```

### What gets optimized

All raster formats: PNG, JPEG, GIF, WebP, BMP, TIFF, AVIF.

SVG and ICO are never resized. SVG is vector (already tiny). ICO has multi-resolution internal structure that should not be modified.

---

## Build

No dependencies. Compiles with any C compiler.

**macOS / Linux:**

```sh
cd img_convert
gcc -O2 -o convert convert.c
```

**macOS (if headers are not found):**

```sh
cc -O2 -o convert convert.c --sysroot="$(xcrun --show-sdk-path)"
```

**Windows (MSVC):**

```
cl convert.c /Fe:convert.exe
```

**Windows (MinGW):**

```
gcc -O2 -o convert.exe convert.c
```

---

## Usage

### Raw data URL (default)

```sh
./convert photo.png
```

Outputs:

```
data:image/png;base64,iVBORw0KGgo...
```

Copy and paste this value anywhere a URL is accepted -- the `src` attribute of an `<img>` tag, a CSS `background-image`, or the `site.image` field in `crissy-data.json`.

### JSON output

```sh
./convert --json avatar.jpg
```

Outputs:

```json
{"file": "avatar.jpg", "mime": "image/jpeg", "size": 24310, "dataUrl": "data:image/jpeg;base64,..."}
```

Multiple files produce a JSON array:

```sh
./convert --json avatar.jpg logo.svg screenshot.png
```

### JSON field (for pasting into crissy-data.json)

```sh
./convert --field site.image avatar.png
```

Outputs:

```
"site.image": "data:image/png;base64,..."
```

### CSS value

```sh
./convert --css background.webp
```

Outputs:

```
url(data:image/webp;base64,...)
```

### HTML img tag

```sh
./convert --html photo.png
```

Outputs:

```html
<img src="data:image/png;base64,..." alt="photo.png">
```

---

## Flags

| Flag | Description |
|------|-------------|
| `--max N` | Max pixel dimension for optimization (default 512). Images larger than NxN are resized to fit, keeping aspect ratio |
| `--no-optimize` | Skip automatic optimization, encode the raw file as-is |
| `--json` | Output each image as a JSON object (or array for multiple files) |
| `--field KEY` | Output as a `"KEY": "data:..."` pair for pasting into JSON |
| `--css` | Output as a CSS `url()` value |
| `--html` | Output as an `<img>` tag |
| `--wrap N` | Line-wrap the base64 at N characters (0 = no wrap, which is the default) |
| `--quiet` | Suppress the info line printed to stderr |
| `--help` | Print usage information |

---

## Supported Formats

PNG, JPEG, GIF, SVG, WebP, ICO, BMP, TIFF, AVIF.

The tool detects the MIME type from the file extension. The actual file contents are encoded as-is regardless of format.

---

## Workflow with the Portfolio

### Using the manage interface

The manage page (`manage.html`) already has a file picker under **Site > Portfolio Image** that converts images to base64 in the browser. Use it when you are working in the manager.

### Using this CLI tool

Use this tool when you want to convert images from the command line, batch-convert multiple files, or script the process.

**Set the portfolio image:**

```sh
./convert photo.png
```

Copy the output and paste it as the value of `"image"` in the `"site"` section of `crissy-data.json`.

**Or use --field to get the exact JSON line:**

```sh
./convert --field image avatar.png >> snippet.json
```

**Pipe into the JSON with jq (if available):**

```sh
DATA_URL=$(./convert --quiet avatar.png)
jq --arg img "$DATA_URL" '.site.image = $img' crissy-data.json > tmp.json && mv tmp.json crissy-data.json
```

### Build and deploy

After updating the JSON, run the build:

```sh
go run build.go .
```

The built HTML files in `build/` will contain the base64 image inline. Push to GitHub and the page is live with the image embedded.

---

## Size Guidelines

| Image type | Recommended max size | Notes |
|------------|---------------------|-------|
| Portfolio avatar | 100-200 KB | 256x256 or 512x512 is plenty |
| Project screenshot | 200-500 KB | Resize to ~800px wide |
| Favicon source | Under 50 KB | 64x64 or 128x128 |
| SVG icons | Under 20 KB | Already small, encodes well |

Base64 adds ~33% to file size. A 150 KB JPEG becomes ~200 KB of base64 text in the HTML. GitHub Pages has no issue serving files up to several megabytes.

---

## License

MIT
