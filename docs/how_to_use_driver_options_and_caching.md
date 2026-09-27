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

Cache keys include the target name, signature, prompt, effective generation
settings, cache salt, resolved system-prompt digest, agent configuration, and
compilation context. The context includes the main source outside prompt bodies,
relevant language, target, and macro flags, and the contents of included headers,
including transitive headers. This deliberately favors safe invalidation: an
unrelated header edit can invalidate a body. Changing output paths or switching
between generated-source output and native compilation does not invalidate it.

Entries record the full key, cache schema, context and prompt digests, model,
and agent identity. Reads validate the metadata; writes use unique temporary
files and an atomic rename. Old prototype entries are ignored and need one
regeneration. Cache metadata does not prove that a generated body is correct.

## Select and limit generation

| Option | Effect |
| --- | --- |
| `-fllm-agent=<command>` | Use a specific external agent command. |
| `-fllm-model=<id>` | Request a model, overriding `LLMCPP_MODEL`. |
| `-fllm-system-prompt=<file>` | Replace the built-in instructions with a UTF-8 file. |
| `-fllm-append-system-prompt=<file>` | Append project instructions; repeat to append several files. |
| `-fllm-agent-config=<file>` | Pass a JSON object to an external agent. |
| `-fllm-max-attempts=<n>` | Limit rejected submissions for one body. |
| `-fllm-max-tool-calls=<n>` | Limit compiler tool calls for one body. |
| `-fllm-timeout=<seconds>` | Set the generation deadline for one body. |

The command-line agent setting takes precedence over automatic backend
selection. Environment-based backend selection is covered in
[Choose an LLM backend](how_to_configure_an_llm_agent.md).

## Configure one function

Use bare `__llm__` for default settings. Add parentheses to supply options:

```cpp
__llm__(model("gpt-6-astra"), max_attempts(2), timeout(120))
void sort_scores(std::vector<int> &scores)
{
    Sort scores from highest to lowest.
}
```

The precedence is built-in defaults, then command-line settings, then target
options. `LLMCPP_MODEL` supplies a model default before command-line parsing.
`no_cache` disables reads and writes for one target. `cache("scores-v1")`
enables caching and adds a salt to its key; the string is not a filename.
These two options cannot be combined. Numeric limits must be positive integer
literals; model names and salts must be nonempty quoted strings.

The agent receives the effective model, cache policy, attempts, tool budget,
and timeout in both `llm/generate` and `get_task`. Limits are enforced by the
compiler. A requested model must be honored or rejected by the agent.
Offline mode still prohibits generation, including for a `no_cache` target.
Existing bare `__llm__` modifiers remain valid. `__llm__()` is also accepted
and has the same effect.

## Control diagnostics

| Option | Effect |
| --- | --- |
| `-fllm-verbose` | Log agent tool calls and short results. |
| `-fllm-quiet` | Hide normal generation progress. |
| `-fllm-transcript=<file>` | Append JSONL generation, tool, and outcome events. |

Start with `-fllm-verbose` when a backend stops without submitting a body or a
candidate is repeatedly rejected.

Verbose output includes the resolved system-prompt digest and agent identity.
Transcripts omit agent configuration and redact credential-bearing JSON fields.
They still contain prompts and code, which may be sensitive; review them before
sharing. Tool results can be read from a transcript without contacting a model,
and replayed with `llmcpp-agent --replay`. See
[How to write an agent](how_to_write_an_agent.md) for the replay command.
