/*
 * beautify.js - Cross-platform text and code beautifier for browser and Node.js
 *
 * Works in:
 *   - Browser (ES5 compatible, no dependencies)
 *   - Node.js via require or import
 *
 * Usage (browser):
 *   <script src="beautify/beautify.js"></script>
 *   Beautify.json('{"a":1}');
 *   Beautify.html('<div><p>Hello</p></div>');
 *   Beautify.css('body{color:red}');
 *   Beautify.extractField(jsonStr, 'explanation');
 *   Beautify.auto(rawText);
 *
 * Usage (Node.js):
 *   var Beautify = require('./beautify/beautify.js');
 *   console.log(Beautify.json('{"a":1}'));
 */

(function(root) {
    'use strict';

    var Beautify = {};

    var DEFAULT_INDENT = 2;

    /* ---- Utility ---- */

    function repeatStr(s, n) {
        var out = '';
        for (var i = 0; i < n; i++) out += s;
        return out;
    }

    function makeIndent(depth, width) {
        return repeatStr(' ', depth * (width || DEFAULT_INDENT));
    }

    function trimStr(s) {
        return s.replace(/^\s+|\s+$/g, '');
    }

    /* ---- JSON Beautifier ---- */

    Beautify.json = function(src, opts) {
        opts = opts || {};
        var indentW = opts.indent || DEFAULT_INDENT;
        var depth = 0;
        var inStr = false;
        var escaped = false;
        var out = '';

        for (var i = 0; i < src.length; i++) {
            var c = src.charAt(i);

            if (escaped) {
                out += c;
                escaped = false;
                continue;
            }

            if (c === '\\' && inStr) {
                out += c;
                escaped = true;
                continue;
            }

            if (c === '"') {
                out += c;
                inStr = !inStr;
                continue;
            }

            if (inStr) {
                out += c;
                continue;
            }

            if (c === ' ' || c === '\t' || c === '\n' || c === '\r') {
                continue;
            }

            if (c === '{' || c === '[') {
                /* Check for empty object or array */
                var j = i + 1;
                while (j < src.length && /\s/.test(src.charAt(j))) j++;
                if (j < src.length &&
                    ((c === '{' && src.charAt(j) === '}') ||
                     (c === '[' && src.charAt(j) === ']'))) {
                    out += c + (c === '{' ? '}' : ']');
                    i = j;
                    continue;
                }
                depth++;
                out += c + '\n' + makeIndent(depth, indentW);
            } else if (c === '}' || c === ']') {
                depth--;
                out += '\n' + makeIndent(depth, indentW) + c;
            } else if (c === ',') {
                out += ',\n' + makeIndent(depth, indentW);
            } else if (c === ':') {
                out += ': ';
            } else {
                out += c;
            }
        }

        return out;
    };

    /* ---- JSON Compact ---- */

    Beautify.jsonCompact = function(src) {
        var inStr = false;
        var escaped = false;
        var out = '';

        for (var i = 0; i < src.length; i++) {
            var c = src.charAt(i);

            if (escaped) { out += c; escaped = false; continue; }
            if (c === '\\' && inStr) { out += c; escaped = true; continue; }
            if (c === '"') { out += c; inStr = !inStr; continue; }
            if (inStr) { out += c; continue; }
            if (c !== ' ' && c !== '\t' && c !== '\n' && c !== '\r') {
                out += c;
            }
        }

        return out;
    };

    /* ---- Extract a top-level JSON string field ---- */

    Beautify.extractField = function(src, fieldName) {
        var needle = '"' + fieldName + '"';
        var pos = src.indexOf(needle);
        if (pos === -1) return null;

        pos += needle.length;
        /* Skip whitespace and colon */
        while (pos < src.length && /[\s:]/.test(src.charAt(pos))) pos++;

        if (src.charAt(pos) !== '"') return null;
        pos++; /* skip opening quote */

        var result = '';
        var esc = false;

        while (pos < src.length) {
            var ch = src.charAt(pos);
            if (esc) {
                if (ch === 'n') result += '\n';
                else if (ch === 't') result += '\t';
                else if (ch === '"') result += '"';
                else if (ch === '\\') result += '\\';
                else if (ch === '/') result += '/';
                else result += '\\' + ch;
                esc = false;
                pos++;
                continue;
            }
            if (ch === '\\') { esc = true; pos++; continue; }
            if (ch === '"') break;
            result += ch;
            pos++;
        }

        return result;
    };

    /* ---- HTML Beautifier ---- */

    Beautify.html = function(src, opts) {
        opts = opts || {};
        var indentW = opts.indent || DEFAULT_INDENT;
        var depth = 0;
        var out = '';
        var inTag = false;
        var atLineStart = true;

        var voids = ['br','hr','img','input','meta','link','area',
                     'base','col','embed','source','track','wbr'];

        for (var i = 0; i < src.length; i++) {
            var c = src.charAt(i);

            if (c === '<') {
                var isClosing = (i + 1 < src.length && src.charAt(i + 1) === '/');
                if (isClosing && depth > 0) depth--;

                if (!atLineStart) out += '\n';
                out += makeIndent(depth, indentW) + c;
                inTag = true;
                atLineStart = false;

                if (!isClosing) {
                    var tagName = '';
                    var ti = i + 1;
                    while (ti < src.length && /[a-zA-Z0-9]/.test(src.charAt(ti))) {
                        tagName += src.charAt(ti);
                        ti++;
                    }
                    tagName = tagName.toLowerCase();
                    var isVoid = false;
                    for (var v = 0; v < voids.length; v++) {
                        if (voids[v] === tagName) { isVoid = true; break; }
                    }
                    if (!isVoid) depth++;
                }
            } else if (c === '>') {
                out += c;
                inTag = false;
                atLineStart = false;
            } else if (c === '\n' || c === '\r') {
                if (!inTag) atLineStart = true;
            } else {
                if (atLineStart && !inTag && (c === ' ' || c === '\t')) continue;
                if (atLineStart && !inTag) {
                    out += '\n' + makeIndent(depth, indentW);
                    atLineStart = false;
                }
                out += c;
            }
        }

        return out;
    };

    /* ---- CSS Beautifier ---- */

    Beautify.css = function(src, opts) {
        opts = opts || {};
        var indentW = opts.indent || DEFAULT_INDENT;
        var depth = 0;
        var inStr = false;
        var strChar = '';
        var out = '';

        for (var i = 0; i < src.length; i++) {
            var c = src.charAt(i);

            if (inStr) {
                out += c;
                if (c === '\\' && i + 1 < src.length) {
                    out += src.charAt(++i);
                    continue;
                }
                if (c === strChar) inStr = false;
                continue;
            }
            if (c === '"' || c === "'") {
                inStr = true;
                strChar = c;
                out += c;
                continue;
            }

            if (c === '\n' || c === '\r') continue;

            if (c === '{') {
                out += ' {\n';
                depth++;
                out += makeIndent(depth, indentW);
            } else if (c === '}') {
                out += '\n';
                depth--;
                if (depth < 0) depth = 0;
                out += makeIndent(depth, indentW) + '}\n';
                if (depth === 0) out += '\n';
            } else if (c === ';') {
                out += ';\n' + makeIndent(depth, indentW);
            } else if (c === ' ' || c === '\t') {
                if (i > 0 && !/[\s{;]/.test(src.charAt(i - 1))) {
                    out += ' ';
                }
            } else {
                out += c;
            }
        }

        return out;
    };

    /* ---- Streaming JSON field extractor ---- */

    /*
     * Creates a streaming parser that extracts only the value of a
     * specific top-level string field from a JSON object being received
     * token by token. Call .feed(chunk) with each piece of text.
     * Provide onContent(text) to get the extracted field value as it
     * streams, and onOther(text) to get notification of other content.
     *
     * Usage:
     *   var extractor = Beautify.createStreamExtractor({
     *       field: 'explanation',
     *       onContent: function(text) { display(text); },
     *       onOther: function(fieldName) { showStatus(fieldName); },
     *       onDone: function(fullContent) { finished(fullContent); }
     *   });
     *   // As streaming chunks arrive:
     *   extractor.feed(chunk);
     *   // When stream ends:
     *   extractor.end();
     */

    Beautify.createStreamExtractor = function(opts) {
        var targetField = opts.field || 'explanation';
        var onContent = opts.onContent || function() {};
        var onOther = opts.onOther || function() {};
        var onDone = opts.onDone || function() {};

        /* Parser states */
        var STATE_SCANNING = 0;     /* Looking for a key */
        var STATE_IN_KEY = 1;       /* Inside a quoted key string */
        var STATE_AFTER_KEY = 2;    /* Between key and colon */
        var STATE_AFTER_COLON = 3;  /* After colon, before value */
        var STATE_IN_TARGET = 4;    /* Inside the target field string value */
        var STATE_IN_OTHER_STR = 5; /* Inside a non-target string value */
        var STATE_IN_OTHER = 6;     /* Inside a non-target non-string value */
        var STATE_DONE = 7;         /* Finished */

        var state = STATE_SCANNING;
        var currentKey = '';
        var collected = '';
        var escaped = false;
        var braceDepth = 0;
        var bracketDepth = 0;
        var otherNotified = false;

        function feed(chunk) {
            for (var i = 0; i < chunk.length; i++) {
                var c = chunk.charAt(i);
                processChar(c);
            }
        }

        function processChar(c) {
            switch (state) {

                case STATE_SCANNING:
                    if (c === '"') {
                        state = STATE_IN_KEY;
                        currentKey = '';
                    }
                    break;

                case STATE_IN_KEY:
                    if (escaped) {
                        currentKey += c;
                        escaped = false;
                    } else if (c === '\\') {
                        escaped = true;
                    } else if (c === '"') {
                        state = STATE_AFTER_KEY;
                    } else {
                        currentKey += c;
                    }
                    break;

                case STATE_AFTER_KEY:
                    if (c === ':') {
                        state = STATE_AFTER_COLON;
                        otherNotified = false;
                    } else if (c !== ' ' && c !== '\t' && c !== '\n' && c !== '\r') {
                        /* Not a key-value pair, go back to scanning */
                        state = STATE_SCANNING;
                    }
                    break;

                case STATE_AFTER_COLON:
                    if (c === ' ' || c === '\t' || c === '\n' || c === '\r') {
                        break; /* skip whitespace */
                    }
                    if (c === '"') {
                        if (currentKey === targetField) {
                            state = STATE_IN_TARGET;
                            collected = '';
                        } else {
                            state = STATE_IN_OTHER_STR;
                            if (!otherNotified) {
                                onOther(currentKey);
                                otherNotified = true;
                            }
                        }
                    } else {
                        /* Non-string value (object, array, number, bool, null) */
                        state = STATE_IN_OTHER;
                        braceDepth = 0;
                        bracketDepth = 0;
                        if (!otherNotified) {
                            onOther(currentKey);
                            otherNotified = true;
                        }
                        /* Process this char as part of the value */
                        processOtherChar(c);
                    }
                    break;

                case STATE_IN_TARGET:
                    if (escaped) {
                        if (c === 'n') { collected += '\n'; onContent('\n'); }
                        else if (c === 't') { collected += '\t'; onContent('\t'); }
                        else if (c === '"') { collected += '"'; onContent('"'); }
                        else if (c === '\\') { collected += '\\'; onContent('\\'); }
                        else if (c === '/') { collected += '/'; onContent('/'); }
                        else { collected += c; onContent(c); }
                        escaped = false;
                    } else if (c === '\\') {
                        escaped = true;
                    } else if (c === '"') {
                        /* End of target field */
                        state = STATE_SCANNING;
                    } else {
                        collected += c;
                        onContent(c);
                    }
                    break;

                case STATE_IN_OTHER_STR:
                    if (escaped) {
                        escaped = false;
                    } else if (c === '\\') {
                        escaped = true;
                    } else if (c === '"') {
                        state = STATE_SCANNING;
                    }
                    break;

                case STATE_IN_OTHER:
                    processOtherChar(c);
                    break;

                case STATE_DONE:
                    break;
            }
        }

        function processOtherChar(c) {
            if (c === '{') braceDepth++;
            else if (c === '}') {
                if (braceDepth > 0) {
                    braceDepth--;
                } else {
                    /* End of outer object */
                    state = STATE_DONE;
                }
            }
            else if (c === '[') bracketDepth++;
            else if (c === ']') {
                if (bracketDepth > 0) bracketDepth--;
            }
            else if (c === ',' && braceDepth === 0 && bracketDepth === 0) {
                state = STATE_SCANNING;
            }
            else if (c === '"') {
                state = STATE_IN_OTHER_STR;
            }
        }

        function end() {
            onDone(collected);
        }

        return {
            feed: feed,
            end: end,
            getCollected: function() { return collected; }
        };
    };

    /* ---- Auto-detect and beautify ---- */

    Beautify.auto = function(src, opts) {
        var trimmed = trimStr(src);
        if (trimmed.charAt(0) === '{' || trimmed.charAt(0) === '[') {
            return Beautify.json(src, opts);
        }
        if (trimmed.charAt(0) === '<') {
            return Beautify.html(src, opts);
        }
        if (/[{};]/.test(trimmed) && !/^[\s]*[<{[]/.test(trimmed)) {
            return Beautify.css(src, opts);
        }
        return src;
    };

    /* ---- Code block formatter for chat display ---- */

    /*
     * Takes text that may contain code blocks (triple backtick fenced)
     * and returns HTML with syntax-highlighted blocks.
     */
    Beautify.formatForChat = function(text) {
        var parts = text.split(/```(\w*)\n?/);
        var html = '';
        var inCode = false;

        for (var i = 0; i < parts.length; i++) {
            if (i % 2 === 1) {
                /* This is the language label */
                inCode = true;
                continue;
            }
            if (inCode) {
                var escaped = parts[i]
                    .replace(/&/g, '&amp;')
                    .replace(/</g, '&lt;')
                    .replace(/>/g, '&gt;');
                html += '<pre style="background:#1a1a2e;color:#e0e0e0;' +
                        'padding:12px;overflow-x:auto;border:1px solid #333;' +
                        'font-family:monospace;font-size:13px;white-space:pre;' +
                        'border-radius:0"><code>' + escaped + '</code></pre>';
                inCode = false;
            } else {
                /* Regular text - convert newlines to <br> */
                var lines = parts[i].split('\n');
                var p = '';
                for (var li = 0; li < lines.length; li++) {
                    var line = lines[li];
                    if (line === '') {
                        if (p) { html += '<p>' + p + '</p>'; p = ''; }
                    } else {
                        if (p) p += '<br>';
                        p += line
                            .replace(/&/g, '&amp;')
                            .replace(/</g, '&lt;')
                            .replace(/>/g, '&gt;');
                    }
                }
                if (p) html += '<p>' + p + '</p>';
            }
        }

        return html;
    };

    /* ---- Export ---- */

    if (typeof module !== 'undefined' && module.exports) {
        module.exports = Beautify;
    } else {
        root.Beautify = Beautify;
    }

})(typeof window !== 'undefined' ? window : this);
