# How to configure an LLM agent

`llmc++` starts `llmcpp-agent` next to its own executable by default. Override
the command with `LLMCPP_AGENT` or `-fllm-agent=<command>` when needed.

The reference agent speaks JSON-RPC/MCP over standard input and output. It has
two backends:

| Backend | Selection | Authentication |
| --- | --- | --- |
| Anthropic API | `LLMCPP_BACKEND=anthropic` | `ANTHROPIC_API_KEY` |
| Claude Code | `LLMCPP_BACKEND=claude-code` | Existing Claude Code login |

When `LLMCPP_BACKEND` is unset, the agent selects the API backend if
`ANTHROPIC_API_KEY` is present, otherwise it uses Claude Code. Set
`LLMCPP_MODEL` to choose a model. Claude Code also accepts `LLMCPP_EFFORT`,
which defaults to `medium`.

For example, use the Anthropic API in the current shell:

```sh
export LLMCPP_BACKEND=anthropic
export ANTHROPIC_API_KEY=your-api-key
llmc++ main.cpp -o main
```

> [!TIP]
> Use `-fllm-dump-context` to inspect the task and compiler context without
> contacting an LLM.
