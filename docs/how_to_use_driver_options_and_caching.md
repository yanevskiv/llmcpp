# How to use driver options and caching

`llmc++` accepts ordinary `clang++` arguments. Its own options control when
generation runs, how much work an agent may do, and what gets written to disk.

## Choose what the driver produces

| Option | Effect |
| --- | --- |
| `--llm` | Write rewritten `<name>.llm.cpp` source and stop. |
| `-fllm-dump` | Print each accepted generated body. |
| `-fllm-dump-context` | Print the task and compiler context without contacting an LLM. |

Without `--llm`, the rewritten translation unit continues through the normal
Clang compilation requested by the remaining command-line arguments.

## Control the cache

Generated bodies are stored in `.llmcache/` beside the source file by default.

| Option | Effect |
| --- | --- |
| `-fllm-offline` | Require cached bodies and never contact an agent. |
| `-fllm-regenerate` | Ignore matching entries and generate fresh bodies. |
| `-fno-llm-cache` | Do not read or write the body cache. |
| `-fllm-cache-dir=<dir>` | Store cache entries in another directory. |

Commit the cache when you want reviewed generated bodies and reproducible
offline builds. A typical CI invocation is:

```sh
llmc++ -fllm-offline main.cpp -o main
```

The prototype cache key covers the target name, signature, and prompt. It does
not fingerprint all surrounding declarations, so regenerate after a context
change that could affect the implementation.

## Select and limit generation

| Option | Effect |
| --- | --- |
| `-fllm-agent=<command>` | Use a specific external agent command. |
| `-fllm-max-attempts=<n>` | Limit rejected submissions for one body. |
| `-fllm-max-tool-calls=<n>` | Limit compiler tool calls for one body. |
| `-fllm-timeout=<seconds>` | Set the generation deadline for one body. |

The command-line agent setting takes precedence over automatic backend
selection. Environment-based backend selection is covered in
[Choose an LLM backend](how_to_configure_an_llm_agent.md).

## Control diagnostics

| Option | Effect |
| --- | --- |
| `-fllm-verbose` | Log agent tool calls and short results. |
| `-fllm-quiet` | Hide normal generation progress. |

Start with `-fllm-verbose` when a backend stops without submitting a body or a
candidate is repeatedly rejected.
