# How to write an agent

An agent is a program that reads and writes newline-delimited JSON-RPC 2.0 on
standard input and output. Launch it with `-fllm-agent='<command>'`. Keep stdout
for protocol messages and send diagnostics to stderr. The compiler starts one
agent process per target; an agent may also handle several requests before EOF.
Socket transports are not implemented.

## Initialize the compiler tools

The agent first sends MCP `initialize`, using protocol version `2025-06-18`,
then `tools/list`, and finally `notifications/initialized`. The initialize
result includes `llmcppProtocolVersion: 1` and `llmcppCapabilities` alongside
the MCP protocol version. MCP tool schemas come from `tools/list`; do not
hard-code their argument shapes.

## Handle generation requests

The compiler sends `llm/generate` with a JSON-RPC request id and these params:

```json
{
  "protocol_version": 1,
  "capabilities": ["compiler_tools", "effective_settings"],
  "name": "sort_scores",
  "signature": "void sort_scores(std::vector<int> &scores)",
  "location": "main.cpp:4:1",
  "prompt": "Sort scores from highest to lowest.",
  "system_prompt": "Resolved compiler instructions...",
  "agent_config": {},
  "context_digest": "SHA-256 of compilation context",
  "generation": {
    "model": "",
    "cache": "enabled",
    "max_attempts": 4,
    "max_tool_calls": 60,
    "timeout_seconds": 600
  },
  "limits": {
    "max_attempts": 4,
    "max_tool_calls": 60,
    "timeout_seconds": 600
  }
}
```

The task also contains AST-derived parameters, return behavior, captures, and
writable outputs, as appropriate to the declaration. `get_task` returns this
task information and the same effective `generation` and `limits` objects.
`system_prompt`, agent configuration, and transport metadata are supplied in
the initial request. Reject unsupported generation protocol versions. Ignore
unknown optional fields so later compatible extensions can add information.

Use the resolved system prompt as the model's instructions. An empty model
means the agent may select its default. A nonempty model requests that model;
honor it or return an error. Do not silently substitute a default when a server
rejects the model. Agent configuration belongs to the adapter: the compiler
validates only that it is a JSON object.

## Deliver a body

While a request is active, the agent sends MCP `tools/call` requests to the
compiler. Correlate replies by id. Call `get_task`, inspect declarations as
needed, check a candidate with `try_compile`, then deliver it through `submit`.
An MCP tool result contains text content and an `isError` flag. A rejected
submission uses another attempt; errors from `try_compile` do not consume the
submission budget. The compiler enforces limits regardless of agent behavior.

After an accepted submission, answer the original `llm/generate` id:

```json
{"jsonrpc":"2.0","id":"llmcpp-1","result":{"status":"ok","model":"served-model"}}
```

On failure, return `status: "error"` and a useful `message`, or a JSON-RPC
error. Merely returning C++ in the final response does not submit a body.
Stop on EOF; the compiler terminates an agent that exceeds its deadline.

## Adapt an open-weight server

[The Python example](../examples/python/example18_agent.py) translates this
protocol to a chat-completions server using only Python's standard library.
Set the server URL and its served model name in
[its configuration](../examples/json/example18_config.json), then run:

```sh
llmc++ -fllm-agent='python3 examples/python/example18_agent.py' \
  -fllm-agent-config=examples/json/example18_config.json \
  examples/example18_custom_agent.cpp -o scores
```

This can connect to Mistral hosted by vLLM, llama.cpp, or another server when
that server implements OpenAI-style chat tool calls. The example expects a
`/v1` base URL and appends `/chat/completions`. Ollama or other servers with a
different tool format need a corresponding translation. API compatibility
alone does not guarantee that a model can use compiler tools reliably.

## Record and replay

Add `-fllm-transcript=trace.jsonl` to record tasks, tool calls, and outcomes.
The bundled agent can replay a successful matching task:

```sh
llmc++ -fllm-regenerate -fllm-agent='llmcpp-agent --replay trace.jsonl' main.cpp
```

Replay checks the recorded context, prompt, instructions, and effective policy,
then runs the recorded calls against the current compiler. It does not contact
a model. Keep the original generation options and source context when replaying.
Agent configuration and credential-bearing fields are redacted; prompts and
tool results may still contain sensitive source information.
