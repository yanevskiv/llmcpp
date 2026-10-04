# llmc++

`llmc++` lets you describe what a C++ function should do instead of writing its
body yourself. Add `__llm__` to a function, method, or lambda, then write the
body in plain language.

When you build the program, an LLM agent turns that description into C++. It
can ask Clang about the declarations, types, members, and captures in scope and
use compiler errors to correct its work. The finished body is compiled with
stock Clang 19, so the resulting executable does not need an LLM at runtime.

Clang can tell whether the generated body is valid C++, but it cannot tell
whether the code does exactly what you meant. You still need to review and test
it. If the function is a template, `llmc++` generates one body for the template
rather than a different body for each instantiation.

Here is a small example:

```cpp
#include <vector>

__llm__ void sort_scores(std::vector<int> &scores)
{
    Sort scores from highest to lowest.
}
```

## Getting started

Create `main.cpp`:

```cpp
#include <iostream>

__llm__ void greet()
{
    Print "Hello from llmc++!" followed by a newline.
}

int main()
{
    greet();
}
```

Fetch dependencies and build `llmc++` once:

```sh
./do_fetch_deps.sh && ./do_build.sh --parallel 8
```

Add the installed tools to this shell's `PATH`.
```
export PATH="$PWD/build/install/bin:$PATH"
```

Provide the API key in the environment inherited by `llmc++`. The OpenAI and
Anthropic API backends are built into the C++ driver and do not start Python.
Alternatively, use `LLMCPP_BACKEND=codex` if you're already logged in with the
Codex CLI; that backend uses the installed Python adapter.
```sh
# export LLMCPP_BACKEND=codex
export LLMCPP_BACKEND=openai
export OPENAI_API_KEY=your-api-key
```

Compile the program and run it:

```sh
 $ llmc++ main.cpp -o main
 $ ./main
Hello from llmcpp!
```
The code was generated at compile time, so `./main` will always print the same text.

Generated functions may return values, and an empty body asks the agent to infer
the conventional behavior from the function name and signature. Comments inside
an `__llm__` body are ignored rather than included in its prompt:

```cpp
__llm__ double sqrt(double x) {
    /* This comment is not visible to the agent. */
}
```

## Examples

| Example | What it shows |
| --- | --- |
| [Hello](examples/01_hello.cpp) | The smallest complete program. |
| [Clamp](examples/02_clamp.cpp) | Parameters and a return value. |
| [Inferred square root](examples/03_square_root.cpp) | Conventional behavior inferred from an empty prompt. |
| [Impossible request](examples/04_impossible_request.cpp) | Compile checks cannot establish whether a request is achievable. |
| [Missing include](examples/05_missing_include.cpp) | Generated bodies cannot use unavailable types. |
| [Composed functions](examples/06_composed_functions.cpp) | Calling one generated function from another. |
| [Inventory reservation](examples/07_inventory.cpp) | Reading and updating private member state. |
| [Interval merging](examples/08_merge_intervals.cpp) | An algorithm over a project-defined record. |
| [Capturing lambda](examples/09_capturing_lambda.cpp) | Using a captured value in an STL algorithm. |
| [Shortest path](examples/10_shortest_path.cpp) | A larger graph algorithm. |
| [Top-k selection](examples/11_top_k_by.cpp) | A range template with a projection. |
| [Projected frequency table](examples/12_projected_frequency_table.cpp) | An iterator template with a dependent return type. |
| [System prompt](examples/13_system_prompt.cpp) | Replacing the compiler's generation instructions. |
| [Project rules](examples/14_append_prompt.cpp) | Appending rules to the built-in instructions. |
| [Model selection](examples/15_model.cpp) | Choosing a model for one function. |
| [Cache policy](examples/16_cache_policy.cpp) | Disabling caching or naming a cache policy per function. |
| [Generation limits](examples/17_generation_limits.cpp) | Setting attempts and timeout for one function. |
| [Custom Python agent](examples/18_custom_agent.cpp) | Connecting a tool-capable local model server through the public protocol. |
| [Transcript replay](examples/19_transcript.cpp) | Recording and replaying compiler tool calls without contacting a model. |
| [Agent configuration](examples/20_agent_configuration.cpp) | Configuring the selected backend with a JSON file. |

## Options

### Environment variables

API credentials and endpoint variables are documented in
[How to configure an LLM agent](docs/how_to_configure_an_llm_agent.md).

Environment variables supply defaults; command-line options override them, followed
by per-function modifiers. Empty values are ignored. Boolean values accept
`1/0`, `true/false`, `yes/no`, or `on/off`. `LLMCPP_CONTEXT` and
`LLMCPP_APPEND_PROMPT` accept one file path or a JSON array of paths, such as
`["reference.md", "types.md"]`; command-line occurrences append to these lists.
Paths are relative to the working directory. Regeneration still overrides offline mode.

| Variable | Effect |
| --- | --- |
| `LLMCPP_BACKEND` | Select `anthropic`, `openai`, `codex`, or `claude`. Required for generation unless a custom agent is supplied. |
| `LLMCPP_AGENT` | Run a custom external agent command; overridden by `-fllm-agent`. |
| `LLMCPP_MODEL` | Set the default model; overridden by `-fllm-model` and per-function `model(...)`. |
| `LLMCPP_SYSTEM_PROMPT` | Replace the built-in system prompt with a UTF-8 file. |
| `LLMCPP_APPEND_PROMPT` | Append one or several UTF-8 files to the system prompt. |
| `LLMCPP_AGENT_CONFIG` | Read a JSON agent configuration object from a file. |
| `LLMCPP_CONTEXT` | Attach one or several UTF-8 reference files. |
| `LLMCPP_OFFLINE` | Use cached bodies only, unless regeneration is enabled. |
| `LLMCPP_REGENERATE` | Ignore cache hits and generate fresh bodies. |
| `LLMCPP_NO_CACHE` | Disable cache reads and writes. |
| `LLMCPP_CACHE_READ_ONLY` | Read cached bodies without writing generated results. |
| `LLMCPP_EXPLAIN_CACHE` | Explain cache decisions on stderr. |
| `LLMCPP_CACHE_DIR` | Choose the cache directory. |
| `LLMCPP_CACHE_SALT` | Set the default salt for computed cache keys. |
| `LLMCPP_CACHE_LIFETIME` | Set the maximum cache age in seconds; `0` means no expiry. |
| `LLMCPP_HASH_ABBREV` | Set the minimum hash length, from `1` to `64`. |
| `LLMCPP_MAX_ATTEMPTS` | Set the positive rejected-submission limit per body. |
| `LLMCPP_MAX_TOOL_CALLS` | Set the positive compiler-tool-call limit per body. |
| `LLMCPP_MAX_OUTPUT_TOKENS` | Set the positive output-token limit per model response. |
| `LLMCPP_TIMEOUT` | Set the positive generation deadline in seconds per body. |
| `LLMCPP_DUMP_CODE` | Print accepted generated bodies. |
| `LLMCPP_DUMP_CONTEXT` | Print task and compiler context without generation. |
| `LLMCPP_VERBOSE` | Print generation progress and agent diagnostics. |
| `LLMCPP_TRANSCRIPT` | Append generation events to a JSONL file. |
| `LLMCPP_EFFORT` | Set reasoning effort for the Codex and Claude Code adapters; default `medium`. |
| `LLMCPP_CODEX` | Choose the Codex executable; default `codex`. |
| `LLMCPP_CLAUDE` | Choose the Claude Code executable; default `claude`. |
| `LLMCPP_DEPS_DIR` | Choose where `do_fetch_deps.sh` downloads dependencies; CMake still expects them under the project's `deps/`. |
| `LLMCPP_MOCK_SCRIPT` | Select the JSON script for `agents/llmcpp-mock-agent`; used for testing. |
| `LLMCPP_MOCK_LOG` | Choose where the mock agent appends its tool-call log. |

### Command line

Boolean switches also accept an explicit value, such as `-fllm-offline=false`
or `-fllm-no-cache=0`, to override environment defaults.

| Option | Effect |
| --- | --- |
| `-fllm` | Accepted for compatibility; `__llm__` generation is already enabled. |
| `-fllm-backend=<backend>` | Select `anthropic`, `openai`, `codex`, or `claude`, overriding `LLMCPP_BACKEND`. |
| `-fllm-agent=<command>` | Run a custom external agent instead of selecting a backend. |
| `-fllm-model=<id>` | Select a model, overriding `LLMCPP_MODEL`. |
| `-fllm-system-prompt=<file>` | Replace the built-in system prompt with a UTF-8 file. |
| `-fllm-append-prompt=<file>` | Append a UTF-8 file to the system prompt; repeat to append several files. |
| `-fllm-agent-config=<file>` | Pass a JSON configuration object to an external agent. |
| `-fllm-context=<file>` | Attach a UTF-8 reference file, separately from system instructions; repeatable. Contents participate in the cache identity. |
| `-fllm-offline` | Use cached bodies only; never contact an agent unless force regeneration is enabled. |
| `-fllm-regenerate` | Ignore cached bodies and generate fresh ones, overriding offline mode. |
| `-fllm-no-cache` | Disable cache reads and writes. |
| `-fllm-cache-read-only` | Allow cache hits and generation on misses, but never write or replace cache entries. Does not enable caching when disabled. |
| `-fllm-explain-cache` | Explain cache hits, misses, bypasses, and write decisions on stderr. |
| `-fllm-cache-dir=<dir>` | Choose the cache directory; defaults to `.llmcache/` beside the source. |
| `-fllm-cache-salt=<salt>` | Add a nonempty default salt to computed cache keys; per-function `cache_salt` replaces it. Does not change caching policy. |
| `-fllm-cache-lifetime=<seconds>` | Treat entries older than this file age as cache misses; `0` means no expiry (default). Offline builds fail on expired entries. |
| `-fllm-hash-abbrev=<n>` | Display at least `n` hash characters and use them in cache filenames (default: 7; range: 1–64). Ambiguous prefixes grow automatically. |
| `-fllm-max-attempts=<n>` | Limit rejected submissions per body; positive integer, default `4`. |
| `-fllm-max-tool-calls=<n>` | Limit compiler tool calls per body; positive integer, default `60`. |
| `-fllm-max-output-tokens=<n>` | Set a positive output-token limit per model response. API backends default to `16000`; Codex and Claude CLI adapters reject explicit limits. Custom agents must enforce the limit or report an error. |
| `-fllm-timeout=<seconds>` | Set the generation deadline per body; positive integer, default `600`. |
| `-fllm-dump-code` | Print accepted generated bodies. |
| `-fllm-dump-context` | Print task and compiler context without contacting an agent. |
| `-fllm-verbose` | Print generation progress, agent tool calls, and short results. |
| `-fllm-transcript=<file>` | Append generation, tool, and outcome events to a JSONL transcript. |
| `--llm` | Write rewritten `<name>.llm.cpp` source and stop instead of compiling it. |
| `-o <file>.cpp`, `-o <file>.cc`, `-o <file>.cxx` | Imply `--llm` and write generated source to the specified file. |

### Modifiers

Combine attributes with commas, for example
`__llm__(model("id"), no_cache, timeout(120))`. They override command-line
defaults; `regenerate` also overrides offline mode. `cache_salt` and `no_cache`
cannot be combined. Model names and cache salts must be nonempty quoted strings;
numeric limits must be positive integer literals, except `cache_lifetime`, which
also accepts `0` for no expiry.

| Modifier | Effect |
| --- | --- |
| `__llm__` | Generate the body using the driver defaults. Parentheses are optional; `__llm__()` has the same effect. |
| `__llm__(backend("name"))` | Override the backend for this function: `anthropic`, `openai`, `codex`, or `claude`. A configured custom agent still takes precedence. |
| `__llm__(agent("command"))` | Override the external agent command for this function; takes precedence over native backends. |
| `__llm__(model("id"))` | Override the model for this function. |
| `__llm__(system_prompt("file"))` | Replace this function's system prompt with a UTF-8 file, excluding driver-appended instructions. Relative paths use the compiler's working directory. |
| `__llm__(append_prompt("file"))` | Append a UTF-8 file to this function's resolved system prompt. Repeat to append several files; applied after `system_prompt` regardless of modifier order. |
| `__llm__(agent_config("file"))` | Replace this function's external-agent configuration with a JSON object from a file. |
| `__llm__(context("file"))` | Attach UTF-8 reference material to this function's task. Repeatable; appended after command-line context files. Relative paths use the working directory; contents participate in the cache identity. |
| `__llm__(offline)` | Use a cached body only for this function; fail on a cache miss unless force regeneration is enabled. Cannot be combined with `no_cache`. |
| `__llm__(regenerate)` | Ignore cached bodies and generate a fresh one for this function, overriding offline mode. Save the result unless caching is disabled. |
| `__llm__(no_cache)` | Disable cache reads and writes for this function. |
| `__llm__(cache_read_only)` | Allow cache reads and generation but prohibit cache writes for this function. |
| `__llm__(explain_cache)` | Explain this function's cache decisions on stderr. |
| `__llm__(cache_dir("path"))` | Override the cache directory for this function. Relative paths use the compiler's working directory; caching policy is unchanged. |
| `__llm__(cache_salt("salt"))` | Replace the default cache salt for this function; the string is not a filename. |
| `__llm__(cache_lifetime(3600))` | Override the maximum cache file age in seconds for this function; `0` means no expiry. Applies even with an explicit `key`. |
| `__llm__(max_attempts(2))` | Override the rejected-submission limit for this function. |
| `__llm__(max_tool_calls(20))` | Override the compiler-tool call limit for this function. |
| `__llm__(max_output_tokens(2048))` | Override the output-token limit per model response for this function; requires a positive integer. Backend restrictions are the same as the command-line option. |
| `__llm__(timeout(120))` | Override the generation deadline in seconds for this function. |
| `__llm__(dump_code)` | Print this function's accepted body, including on cache hits. |
| `__llm__(dump_context)` | Print this function's task and compiler context. Stops the compilation without generating any bodies; only selected functions are printed. |
| `__llm__(verbose)` | Print progress and agent tool diagnostics for this function. |
| `__llm__(transcript("file"))` | Override the JSONL transcript destination for this function. Relative paths use the compiler's working directory. |
| `__llm__(key("hash"))` | Use an explicit cache identity (7 to 64 hexadecimal characters). An unambiguous prefix can select an existing body despite changed inputs; a miss generates and saves under this key. Cannot be combined with `no_cache`. |

### Macros

| Macro | Effect |
| --- | --- |
| `__LLMCPP__` | Defined as `1` when compiling or preprocessing with llmc++. Use `#ifdef __LLMCPP__` to distinguish llmc++ from other compilers. |

## Technologies

- **Languages**: C++20 for llmc++ and its tests, Python 3, shell.
- **Compiler infrastructure**: LLVM 19, Clang 19.
- **Build and testing**: CMake, CTest, Catch2.
- **Networking**: cpp-httplib, OpenSSL.
- **Containers**: Docker, Buildx.
- **Code quality**: clang-format, clang-tidy, pre-commit.

## Requirements

On Debian or Ubuntu x86-64 with CMake 3.20 or newer and LLVM/Clang 19 packages
available, install the build and test prerequisites:

```sh
sudo apt update
sudo apt install build-essential
sudo apt install cmake
sudo apt install python3
sudo apt install curl
sudo apt install ca-certificates
```

To build documentation separately, install its tools:

```sh
sudo apt install doxygen
sudo apt install python3-venv
./do_build_docs.sh
```

Fetch LLVM/Clang 19, OpenSSL, Catch2, and cpp-httplib locally into `deps/`:

```sh
./do_fetch_deps.sh
```

For development checks, also install Git, pre-commit, and the Clang tools:

```sh
sudo apt install git
sudo apt install pre-commit
sudo apt install clang-format-19
sudo apt install clang-tidy-19
```

The check scripts expect `clang-format` and `clang-tidy` on `PATH`; if your
distribution only provides versioned commands, add symlinks or wrappers with
those names. Live generation also needs a configured
[LLM backend](docs/how_to_configure_an_llm_agent.md); tests use mock agents.

## Learn more

Start with the [documentation homepage](docs/web/index.md) for the project overview,
numbered tutorials, task-focused guides, and contributor API reference.

- [How to build, test, and clean](docs/how_to_build_test_and_clean.md)
- [How LLM compilation works](docs/how_llm_compilation_works.md)
- [How to configure an LLM agent](docs/how_to_configure_an_llm_agent.md)
- [How to write an agent](docs/how_to_write_an_agent.md)
- [How to use driver options and caching](docs/how_to_use_driver_options_and_caching.md)
- [How the source is structured](docs/how_the_source_is_structured.md)
- [How to work on llmcpp](docs/how_to_work_on_llmcpp.md)

## Author

Ivan Janevski (C) 2026
