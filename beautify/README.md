# Beautify Module

Cross-platform text and code beautifier for AI agent output formatting.

## Contents

- **beautify.js** - Browser and Node.js module (ES5, no dependencies)
- **beautify.c** - Native C command-line tool (macOS, Linux, Windows)

## JavaScript API

Include in a browser page or require in Node.js:

```html
<script src="beautify/beautify.js"></script>
```

```javascript
var Beautify = require('./beautify/beautify.js');
```

### Functions

| Function | Description |
|---|---|
| `Beautify.json(src, opts)` | Format JSON with indentation |
| `Beautify.jsonCompact(src)` | Minify JSON |
| `Beautify.html(src, opts)` | Format HTML with indentation |
| `Beautify.css(src, opts)` | Format CSS with indentation |
| `Beautify.extractField(src, fieldName)` | Extract a top-level string field from JSON |
| `Beautify.auto(src, opts)` | Auto-detect format and beautify |
| `Beautify.formatForChat(text)` | Convert text with code blocks to HTML |
| `Beautify.createStreamExtractor(opts)` | Create a streaming JSON field extractor |

### Options

All formatters accept `{ indent: N }` to set indent width (default: 2).

### Streaming Extractor

For AI agent chat streaming, the stream extractor parses JSON as it
arrives token by token and only shows the content of a specified field
to the user:

```javascript
var extractor = Beautify.createStreamExtractor({
    field: 'explanation',
    onContent: function(text) {
        // Display this text to the user in real time
        chatDiv.textContent += text;
    },
    onOther: function(fieldName) {
        // Another field is being received (e.g. "data")
        statusDiv.textContent = 'Applying changes...';
    },
    onDone: function(fullContent) {
        // Stream finished
    }
});

// Feed chunks as they arrive from the API:
extractor.feed(chunk);

// When stream ends:
extractor.end();
```

## C Command-Line Tool

### Build

```bash
# macOS
cc -O2 -o beautify beautify.c --sysroot="$(xcrun --show-sdk-path)"

# Linux
gcc -O2 -o beautify beautify.c

# Windows (MSVC)
cl beautify.c /Fe:beautify.exe

# Windows (MinGW)
gcc -O2 -o beautify.exe beautify.c
```

### Usage

```bash
echo '{"a":1,"b":[2,3]}' | ./beautify --json
./beautify --json data.json
./beautify --extract-field explanation response.json
./beautify --html page.html
./beautify --css styles.css
./beautify --compact --json data.json
./beautify --indent 4 --json data.json
```
