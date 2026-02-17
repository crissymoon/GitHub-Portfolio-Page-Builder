package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

// build.go - CLI build tool for Crissy Portfolio
// Inlines styles.css, scripts.js, and crissy-data.json into a single HTML file
// then copies that HTML to all configured output filenames.

func main() {
	dir := "."
	if len(os.Args) > 1 {
		dir = os.Args[1]
	}

	htmlPath := filepath.Join(dir, "index.html")
	cssPath := filepath.Join(dir, "styles.css")
	jsPath := filepath.Join(dir, "scripts.js")
	jsonPath := filepath.Join(dir, "crissy-data.json")

	htmlBytes, err := os.ReadFile(htmlPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading index.html: %v\n", err)
		os.Exit(1)
	}
	cssBytes, err := os.ReadFile(cssPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading styles.css: %v\n", err)
		os.Exit(1)
	}
	jsBytes, err := os.ReadFile(jsPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading scripts.js: %v\n", err)
		os.Exit(1)
	}
	jsonBytes, err := os.ReadFile(jsonPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading crissy-data.json: %v\n", err)
		os.Exit(1)
	}

	html := string(htmlBytes)
	css := string(cssBytes)
	js := string(jsBytes)
	jsonData := string(jsonBytes)

	// Replace the external CSS link with inline style
	inlineCSS := "<style>\n" + css + "\n</style>"
	html = replaceLinkTag(html, inlineCSS)

	// Replace the external JS script with inline script that embeds JSON data
	inlineJS := "<script>\nvar __CRISSY_DATA__ = " + jsonData + ";\n</script>\n"
	inlineJS += "<script>\n" + patchJSForInline(js) + "\n</script>"
	html = replaceScriptTag(html, inlineJS)

	// Output files: the 4 pages that should all be identical
	outputFiles := []string{
		"index.html",
		"projects.html",
		"links.html",
		"about.html",
	}

	buildDir := filepath.Join(dir, "build")
	err = os.MkdirAll(buildDir, 0755)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating build directory: %v\n", err)
		os.Exit(1)
	}

	for _, name := range outputFiles {
		outPath := filepath.Join(buildDir, name)
		err = os.WriteFile(outPath, []byte(html), 0644)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error writing %s: %v\n", name, err)
			os.Exit(1)
		}
		fmt.Printf("Built: %s\n", outPath)
	}

	fmt.Println("Build complete.")
}

// replaceLinkTag replaces <link rel="stylesheet" href="styles.css"> with inline CSS
func replaceLinkTag(html, inlineCSS string) string {
	// Find and replace the stylesheet link
	lines := strings.Split(html, "\n")
	var result []string
	for _, line := range lines {
		trimmed := strings.TrimSpace(line)
		if strings.Contains(trimmed, "styles.css") && strings.Contains(trimmed, "<link") {
			result = append(result, inlineCSS)
		} else {
			result = append(result, line)
		}
	}
	return strings.Join(result, "\n")
}

// replaceScriptTag replaces <script src="scripts.js"></script> with inline JS
func replaceScriptTag(html, inlineJS string) string {
	lines := strings.Split(html, "\n")
	var result []string
	for _, line := range lines {
		trimmed := strings.TrimSpace(line)
		if strings.Contains(trimmed, "scripts.js") && strings.Contains(trimmed, "<script") {
			result = append(result, inlineJS)
		} else {
			result = append(result, line)
		}
	}
	return strings.Join(result, "\n")
}

// patchJSForInline modifies the JS to use embedded data instead of XHR
func patchJSForInline(js string) string {
	// Replace the loadData function to use the embedded __CRISSY_DATA__ variable
	replacement := `function loadData(callback) {
    try {
      siteData = __CRISSY_DATA__;
      callback(null, siteData);
    } catch (e) {
      callback(e, null);
    }
  }`

	// Find the loadData function and replace it
	startMarker := "function loadData(callback) {"
	startIdx := strings.Index(js, startMarker)
	if startIdx == -1 {
		return js
	}

	// Find the matching closing brace by counting braces
	braceCount := 0
	endIdx := startIdx
	for i := startIdx; i < len(js); i++ {
		if js[i] == '{' {
			braceCount++
		} else if js[i] == '}' {
			braceCount--
			if braceCount == 0 {
				endIdx = i + 1
				break
			}
		}
	}

	return js[:startIdx] + replacement + js[endIdx:]
}
