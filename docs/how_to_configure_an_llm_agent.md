# How to configure an LLM agent

`llmc++` has a native Anthropic Messages API client. It uses `cpp-httplib`,
OpenSSL, and the compiler's existing JSON and tool implementations, so this
path does not start the Python `llmcpp-agent` program.

The backend choices are:

| Backend | Selection | Implementation | Authentication |
| --- | --- | --- | --- |
| Anthropic API | `LLMCPP_BACKEND=anthropic` | Native C++ HTTPS client | `ANTHROPIC_API_KEY` |
| Claude Code | `LLMCPP_BACKEND=claude-code` | Python adapter and `claude` CLI | Existing Claude Code login |

When `LLMCPP_BACKEND` is unset or `auto`, `llmc++` uses its native API client
if `ANTHROPIC_API_KEY` is present. Otherwise it starts the adjacent
`llmcpp-agent`, which selects Claude Code when available. Set `LLMCPP_MODEL` to
choose a model. Claude Code also accepts `LLMCPP_EFFORT`, which defaults to
`medium`. `ANTHROPIC_BASE_URL` may override the API origin for a compatible
endpoint.

Set `LLMCPP_AGENT` or `-fllm-agent=<command>` to force an external agent. An
explicit external-agent setting takes precedence over native backend selection.
The command must speak the project's newline-delimited JSON-RPC/MCP protocol
over standard input and output. This mechanism is used by the Claude Code
adapter and by the deterministic test agent.

For example, use the Anthropic API in the current shell:

```sh
export LLMCPP_BACKEND=anthropic
export ANTHROPIC_API_KEY=your-api-key
llmc++ main.cpp -o main
```

Use an existing Claude Code login instead:

```sh
export LLMCPP_BACKEND=claude-code
llmc++ main.cpp -o main
```

> [!TIP]
> Use `-fllm-dump-context` to inspect the task and compiler context without
> contacting an LLM.
