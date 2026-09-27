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
./fetch-deps.sh && ./build.sh --parallel 8
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
| [Hello](examples/example01_hello.cpp) | The smallest complete program. |
| [Clamp](examples/example02_clamp.cpp) | Parameters and a return value. |
| [Inferred square root](examples/example03_square_root.cpp) | Conventional behavior inferred from an empty prompt. |
| [Impossible request](examples/example04_impossible_request.cpp) | Compile checks cannot establish whether a request is achievable. |
| [Missing include](examples/example05_missing_include.cpp) | Generated bodies cannot use unavailable types. |
| [Composed functions](examples/example06_composed_functions.cpp) | Calling one generated function from another. |
| [Inventory reservation](examples/example07_inventory.cpp) | Reading and updating private member state. |
| [Interval merging](examples/example08_merge_intervals.cpp) | An algorithm over a project-defined record. |
| [Capturing lambda](examples/example09_capturing_lambda.cpp) | Using a captured value in an STL algorithm. |
| [Shortest path](examples/example10_shortest_path.cpp) | A larger graph algorithm. |
| [Top-k selection](examples/example11_top_k_by.cpp) | A range template with a projection. |
| [Projected frequency table](examples/example12_projected_frequency_table.cpp) | An iterator template with a dependent return type. |
| [System prompt](examples/example13_system_prompt.cpp) | Replacing the compiler's generation instructions. |
| [Project rules](examples/example14_append_system_prompt.cpp) | Appending rules to the built-in instructions. |
| [Model selection](examples/example15_model.cpp) | Choosing a model for one function. |
| [Cache policy](examples/example16_cache_policy.cpp) | Disabling caching or naming a cache policy per function. |
| [Generation limits](examples/example17_generation_limits.cpp) | Setting attempts and timeout for one function. |
| [Custom Python agent](examples/example18_custom_agent.cpp) | Connecting a tool-capable local model server through the public protocol. |
| [Transcript replay](examples/example19_transcript.cpp) | Recording and replaying compiler tool calls without contacting a model. |
| [Agent configuration](examples/example20_agent_configuration.cpp) | Selecting a backend and its options in a JSON file. |

## Learn more

- [How to build, test, and clean](docs/how_to_build_test_and_clean.md)
- [How LLM compilation works](docs/how_llm_compilation_works.md)
- [How to configure an LLM agent](docs/how_to_configure_an_llm_agent.md)
- [How to write an agent](docs/how_to_write_an_agent.md)
- [How to use driver options and caching](docs/how_to_use_driver_options_and_caching.md)
- [How the source is structured](docs/how_the_source_is_structured.md)
- [How to develop llmc++](docs/how_to_develop_llmcxx.md)
- [Prototype plan and milestones](PLAN.md)

## Author

Ivan Janevski (C) 2026
