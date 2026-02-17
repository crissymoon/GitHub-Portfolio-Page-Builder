# cmds -- Agent Command Runner

A cross-platform C tool that lets an AI agent execute terminal commands through a human approval gate. The agent requests a command, the operator sees exactly what will run, and nothing executes until the operator types `y`.

---

## Why This Exists

When an AI agent has tool-call access to run shell commands, every invocation is a potential risk. This tool sits between the agent and the operating system. It prints the full command, waits for explicit approval, and logs every request regardless of whether it was approved or rejected. The agent never runs anything silently.

---

## How It Works

1. The agent (or any caller) invokes `run` with a command
2. The tool prints the command to stderr and prompts for approval
3. The operator reads the command and types `y` to approve or anything else to reject
4. If approved, the command runs via the system shell and the exit code is captured
5. A structured JSON result is printed to stdout for the caller to parse
6. If a log file is configured, every request is recorded with a timestamp

---

## Build

No dependencies. Compiles with any C compiler on any platform.

**macOS / Linux:**

```sh
cd cmds
gcc -O2 -o run run.c
```

**macOS (if headers are not found):**

```sh
cc -O2 -o run run.c --sysroot="$(xcrun --show-sdk-path)"
```

**Windows (MSVC):**

```
cl run.c /Fe:run.exe
```

**Windows (MinGW):**

```
gcc -O2 -o run.exe run.c
```

---

## Usage

### Direct command

```sh
./run ls -la /tmp
```

The tool assembles the arguments into a single command string, shows the approval prompt, and runs it if approved.

### Quoted command

```sh
./run "git status && git log --oneline -5"
```

### JSON mode (for agent tool-call protocols)

```sh
echo '{"cmd":"uname -a"}' | ./run --json
```

Reads a JSON object from stdin with a `"cmd"` field. The approval prompt still appears on the terminal because it reads from `/dev/tty` (Unix) or `CON` (Windows), not stdin.

---

## Flags

| Flag | Description |
|------|-------------|
| `--json` | Read the command from a JSON object on stdin instead of argv |
| `--yes` | Skip the approval prompt entirely (use only in trusted, locked-down pipelines) |
| `--timeout N` | Seconds to wait for operator input (0 = wait forever, which is the default) |
| `--log FILE` | Append every request to FILE with a timestamp and approval status |
| `--help` | Print usage information |

---

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Command executed and returned exit code 0 |
| 1 | Command executed but returned a non-zero exit code |
| 2 | Operator rejected the command |
| 3 | Usage error (no command provided, bad JSON, etc.) |

---

## Output Format

The tool prints a JSON line to stdout after every invocation so the calling agent can parse the result programmatically.

**Approved and executed:**

```json
{"status":"executed","exit_code":0,"command":"ls -la"}
```

**Rejected by operator:**

```json
{"status":"rejected","command":"rm -rf /"}
```

The command's own stdout and stderr pass through to the terminal normally. The structured JSON result is a separate line printed after execution completes.

---

## Log File

When `--log` is specified, every request is appended to the file with this format:

```
[2026-02-17 14:32:01] APPROVED | git status
[2026-02-17 14:32:01] SUCCESS  | git status
[2026-02-17 14:33:15] REJECTED | rm -rf /
```

Each request produces two log entries when approved (APPROVED then SUCCESS or FAILED) and one entry when rejected (REJECTED).

---

## Approval Process

The approval gate is the core safety mechanism. Here is how it works:

1. The command is displayed in full on stderr so the operator can inspect it
2. The prompt reads from the controlling terminal (`/dev/tty` on Unix, `CON` on Windows), not from stdin -- this means JSON mode and piped input do not bypass the prompt
3. Only `y` or `Y` (optionally followed by other characters) counts as approval
4. An empty line, `n`, `N`, EOF, or any other input counts as rejection
5. If the terminal is not available and `--yes` was not passed, the command is rejected by default

The `--yes` flag disables the gate entirely. It exists for CI/CD or other environments where a human has already vetted the command set. Do not use it in interactive agent sessions.

---

## Integration with AI Agents

### MCP / Function Calling

Define a tool that shells out to `run`:

```json
{
  "name": "run_command",
  "description": "Execute a shell command on the host. Requires operator approval.",
  "parameters": {
    "type": "object",
    "properties": {
      "cmd": {
        "type": "string",
        "description": "The shell command to execute"
      }
    },
    "required": ["cmd"]
  }
}
```

The agent sends `{"cmd": "..."}`, your orchestrator pipes it to `./run --json`, and the operator approves or rejects.

### Direct Subprocess

Any system that can spawn a process can call `run` directly:

```
./run --log agent.log git push origin main
```

The log file provides an audit trail of everything the agent attempted.

---

## Security Notes

- The tool does not sanitize or filter commands. The operator is the filter.
- The `--yes` flag removes the only safety gate. Use it with extreme caution.
- The log file is append-only from the tool's perspective but has no file locking. In single-agent scenarios this is fine.
- The JSON parser is minimal and does not handle nested objects or arrays. The `"cmd"` value must be a simple string.
- Commands run with the same privileges as the user who started the tool. There is no sandboxing.

---

## License

MIT
