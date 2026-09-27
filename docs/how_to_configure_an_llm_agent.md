# How to configure an LLM agent

You have a function ready to generate; now choose who will generate it.
`llmc++` can use the OpenAI or Anthropic API, Codex, or Claude Code. If you
already use one of the CLI tools, start by reusing that login. If you need an
API endpoint or your own agent process, the sections below explain those routes.

| Backend | Select it with | What you need |
| --- | --- | --- |
| OpenAI API | `LLMCPP_BACKEND=openai` | `OPENAI_API_KEY` |
| Codex | `LLMCPP_BACKEND=codex` | An installed and logged-in `codex` CLI |
| Claude Code | `LLMCPP_BACKEND=claude` | An installed and logged-in `claude` CLI |
| Anthropic API | `LLMCPP_BACKEND=anthropic` | `ANTHROPIC_API_KEY` |

## Use Codex

After installing the [Codex CLI](https://developers.openai.com/codex/cli/reference)
and logging in, select it in the shell where you compile:

```sh
export LLMCPP_BACKEND=codex
llmc++ main.cpp -o main
```

The Python adapter starts `codex exec` once for each body that needs to be
generated. It gives Codex a temporary read-only workspace and exposes the
compiler-context tools through a temporary MCP server. Your normal Codex login
is available, but user configuration is not loaded and the temporary workspace
does not contain your project files.

Set `LLMCPP_MODEL` to a model identifier accepted by your Codex installation.
Set `LLMCPP_EFFORT` when you want to override the reasoning effort:

```sh
export LLMCPP_EFFORT=high
```

Set `LLMCPP_CODEX` if the executable is not named `codex` or is not on `PATH`.

## Use the OpenAI API

The OpenAI client runs inside the C++ driver and uses the
[Responses API](https://developers.openai.com/api/docs/guides/function-calling)
with the compiler tools exposed as function tools:

```sh
export LLMCPP_BACKEND=openai
export OPENAI_API_KEY=your-api-key
llmc++ main.cpp -o main
```

`LLMCPP_MODEL` selects the model. If it is unset, the prototype uses
`gpt-6-astra`. `OPENAI_BASE_URL` can point the client at an API-compatible
endpoint.

## Use Claude Code

Select Claude Code in the same way:

```sh
export LLMCPP_BACKEND=claude
llmc++ main.cpp -o main
```

This backend starts `claude -p` and gives it the same compiler tools through
MCP. `LLMCPP_MODEL` and `LLMCPP_EFFORT` are passed to the CLI. Set
`LLMCPP_CLAUDE` to use a different executable.

## Use the Anthropic API

The Anthropic client runs inside the C++ driver, so it does not start Python or
a separate CLI:

```sh
export LLMCPP_BACKEND=anthropic
export ANTHROPIC_API_KEY=your-api-key
llmc++ main.cpp -o main
```

`LLMCPP_MODEL` selects the model. If it is unset, the prototype uses
`claude-opus-5`. `ANTHROPIC_BASE_URL` can point the client at an API-compatible
endpoint.

## Select a backend explicitly

Set `LLMCPP_BACKEND` or pass `-fllm-backend=<backend>`; the command-line option
takes precedence. Supported backends are `anthropic`, `openai`, `codex`, and
`claude`. API credentials and
installed CLI programs are used only after the backend has been selected.

```sh
llmc++ -fllm-backend=claude main.cpp -o main
```

Generation fails if no backend or custom agent is supplied. Ordinary C++
compilation and context inspection do not need a backend. Cached bodies are
separated by the selected backend.

## Use a custom agent

`-fllm-agent=<command>` or `LLMCPP_AGENT=<command>` bypasses backend selection
and starts that command instead. A custom agent communicates with `llmc++`
through newline-delimited JSON-RPC over standard input and output. It acts as
an MCP client: it lists and calls the compiler tools, then returns the result of
the `llm/generate` request.

The deterministic `agents/llmcpp-mock-agent` uses this interface in the
integration suite.

The public generation protocol is version 1. See
[How to write an agent](how_to_write_an_agent.md) for its messages and a Python
adapter for tool-capable local model servers.

## Supply instructions and configuration

`-fllm-system-prompt=PROMPT.md` replaces the built-in system instructions.
`-fllm-append-prompt=RULES.md` appends rules after those instructions;
repeat it to append multiple files in command-line order. Prompt files must
contain UTF-8 text. A replacement prompt should still explain the compiler
tools and require an accepted `submit`.

`-fllm-model=<id>` overrides the environment model default. A function can
override it again with `__llm__(model("id"))`. The selected agent must honor
the requested model or report a failure.

`-fllm-agent-config=CONFIG.json` passes a JSON object to an external agent and
selects the Python adapter instead of a native API client. For the bundled
adapter, supported fields are `model`, `effort`, `executable`,
`base_url`, and `api_key`; only fields relevant to its selected backend apply.
The bundled adapter supports all four backends, including OpenAI through the
Responses API. Configuration does not select a backend: use `LLMCPP_BACKEND`
or `-fllm-backend`. For example, `{"effort":"high"}` configures reasoning effort
for an explicitly selected CLI backend. Prefer environment variables for credentials.
Custom agents define their own configuration fields.

> [!TIP]
> Run `llmc++ -fllm-dump-context source.cpp` to inspect the task and compiler
> context without contacting any backend.
