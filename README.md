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
| [Project rules](examples/14_append_system_prompt.cpp) | Appending rules to the built-in instructions. |
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

| Variable | Effect |
| --- | --- |
| `LLMCPP_BACKEND` | Select `anthropic`, `openai`, `codex`, or `claude`. Required for generation unless a custom agent is supplied. |
| `LLMCPP_AGENT` | Run a custom external agent command; overridden by `-fllm-agent`. |
| `LLMCPP_MODEL` | Set the default model; overridden by `-fllm-model` and per-function `model(...)`. |
| `LLMCPP_EFFORT` | Set reasoning effort for the Codex and Claude Code adapters; default `medium`. |
| `LLMCPP_CODEX` | Choose the Codex executable; default `codex`. |
| `LLMCPP_CLAUDE` | Choose the Claude Code executable; default `claude`. |
| `LLMCPP_VERBOSE` | Enable Python adapter diagnostics when nonempty; also set by `-fllm-verbose`. |
| `LLMCPP_DEPS_DIR` | Choose where `do_fetch_deps.sh` downloads dependencies; CMake still expects them under the project's `deps/`. |
| `LLMCPP_MOCK_SCRIPT` | Select the JSON script for `agents/llmcpp-mock-agent`; used for testing. |
| `LLMCPP_MOCK_LOG` | Choose where the mock agent appends its tool-call log. |

### Command line

| Option | Effect |
| --- | --- |
| `-fllm` | Accepted for compatibility; `__llm__` generation is already enabled. |
| `-fllm-backend=<backend>` | Select `anthropic`, `openai`, `codex`, or `claude`, overriding `LLMCPP_BACKEND`. |
| `-fllm-agent=<command>` | Run a custom external agent instead of selecting a backend. |
| `-fllm-model=<id>` | Select a model, overriding `LLMCPP_MODEL`. |
| `-fllm-system-prompt=<file>` | Replace the built-in system prompt with a UTF-8 file. |
| `-fllm-append-system-prompt=<file>` | Append a UTF-8 file to the system prompt; repeat to append several files. |
| `-fllm-agent-config=<file>` | Pass a JSON configuration object to an external agent. |
| `-fllm-offline` | Use cached bodies only; never contact an agent. |
| `-fllm-regenerate` | Ignore cached bodies and generate fresh ones. |
| `-fllm-no-cache` | Disable cache reads and writes. |
| `-fllm-cache-dir=<dir>` | Choose the cache directory; defaults to `.llmcache/` beside the source. |
| `-fllm-hash-abbrev=<n>` | Display at least `n` hash characters and use them in cache filenames (default: 7; range: 1–64). Ambiguous prefixes grow automatically. |
| `-fllm-max-attempts=<n>` | Limit rejected submissions per body; positive integer, default `4`. |
| `-fllm-max-tool-calls=<n>` | Limit compiler tool calls per body; positive integer, default `60`. |
| `-fllm-timeout=<seconds>` | Set the generation deadline per body; positive integer, default `600`. |
| `-fllm-dump` | Print accepted generated bodies. |
| `-fllm-dump-context` | Print task and compiler context without contacting an agent. |
| `-fllm-verbose` | Print generation progress, agent tool calls, and short results. |
| `-fllm-transcript=<file>` | Append generation, tool, and outcome events to a JSONL transcript. |
| `--llm` | Write rewritten `<name>.llm.cpp` source and stop instead of compiling it. |
| `-o <file>.cpp`, `-o <file>.cc`, `-o <file>.cxx` | Imply `--llm` and write generated source to the specified file. |

### Modifiers

Combine attributes with commas, for example
`__llm__(model("id"), no_cache, timeout(120))`. They override command-line
defaults, but cannot enable generation in offline mode. `cache` and `no_cache`
cannot be combined. Model names and cache salts must be nonempty quoted strings;
numeric limits must be positive integer literals.

| Modifier | Effect |
| --- | --- |
| `__llm__` | Generate the body using the driver defaults. Parentheses are optional; `__llm__()` has the same effect. |
| `__llm__(model("id"))` | Override the model for this function. |
| `__llm__(backend("name"))` | Override the backend for this function: `anthropic`, `openai`, `codex`, or `claude`. A configured custom agent still takes precedence. |
| `__llm__(agent("command"))` | Override the external agent command for this function; takes precedence over native backends. |
| `__llm__(no_cache)` | Disable cache reads and writes for this function. |
| `__llm__(offline)` | Use a cached body only for this function, even with `-fllm-regenerate`; fail on a cache miss. Cannot be combined with `no_cache`. |
| `__llm__(key("hash"))` | Use an explicit cache identity (7 to 64 hexadecimal characters). An unambiguous prefix can select an existing body despite changed inputs; a miss generates and saves under this key. Cannot be combined with `no_cache`. |
| `__llm__(cache("salt"))` | Enable caching and add a salt to this function's cache key; the string is not a filename. |
| `__llm__(max_attempts(2))` | Override the rejected-submission limit for this function. |
| `__llm__(timeout(120))` | Override the generation deadline in seconds for this function. |
| `__llm__(dump)` | Print this function's accepted body, including on cache hits. |

### Macros

| Macro | Effect |
| --- | --- |
| `__LLMCPP__` | Defined as `1` when compiling or preprocessing with llmc++. Use `#ifdef __LLMCPP__` to distinguish llmc++ from other compilers. |

## Technologies

- **Languages**: C++17, Python 3, shell.
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
