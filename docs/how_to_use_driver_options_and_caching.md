# How to use driver options and caching

`llmc++` accepts normal `clang++` options plus the following options:

| Option | Meaning |
| --- | --- |
| `--llm` | Write rewritten `<name>.llm.cpp` source and stop. |
| `-fllm-agent=<command>` | Select the agent command. |
| `-fllm-offline` | Use cached bodies only. |
| `-fllm-regenerate` | Ignore cached bodies and generate new ones. |
| `-fno-llm-cache` | Do not read or write the body cache. |
| `-fllm-cache-dir=<dir>` | Select a cache directory. |
| `-fllm-dump` | Print accepted generated bodies. |
| `-fllm-dump-context` | Print task and context information without contacting an LLM. |
| `-fllm-verbose` | Log tool calls. |
| `-fllm-quiet` | Suppress generation progress. |
| `-fllm-max-attempts=<n>` | Limit rejected candidate submissions per body. |
| `-fllm-max-tool-calls=<n>` | Limit agent tool calls per body. |
| `-fllm-timeout=<seconds>` | Set a per-body generation timeout. |

Generated bodies are stored in `.llmcache/` next to the source file by default.
Commit a cache when you want reproducible offline builds, then use
`-fllm-offline` in CI.
