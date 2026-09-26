# llmc++

`llmc++` is a prototype C++ compiler driver, based on Clang, that adds an `__llm__` function specifier.
It's somewhat inspired by CUDA's `__global__`, which marks those functions which should run on the GPU.
Except when you mark a function, method, or a lambda with `__llm__` you can write its body in a natural language rather than strict C++

`llmc++` asks an LLM agent to generate the real C++ body, then compiles the result with stock Clang 19.
The LLM agent doesn't look at the raw source code; rather it looks at Clang's compilation context.
This gives it a unique ability to know what template types are.

For example, the LLM will know what T is.
```c++
// $ llmc++ main.c -o main
// $ ./main
#include <iostream>

template <typename T> __llm__ void f()
{
    Use std::cout to print type is T is here.
}

int main() {
    f<int>();
    f<short>();
}
```

## Getting started

Create `main.cpp`:

```cpp
#include <iostream>

__llm__ void greet()
{
    Print "Hello from llmcpp!" followed by a newline.
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

Provide the API key in the environment inherited by `llmc++`. The Anthropic API
backend is built into the C++ driver and does not start Python. Alternatively,
use `LLMCPP_BACKEND=claude-code` if you're already logged in with Claude Code;
that backend uses the installed Python adapter to launch the `claude` CLI.
```sh
# export LLMCPP_BACKEND=claude-code
export LLMCPP_BACKEND=anthropic
export ANTHROPIC_API_KEY=your-api-key
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

[`examples/`](examples/) contains four numbered examples: a hello function, a
type-aware template, an empty inferred square-root function, and a constrained
return-value helper. For example:

```sh
llmc++ examples/example01_hello.cpp -o hello
./hello
```

## Learn more

- [How to build, test, and clean](docs/how_to_build_test_and_clean.md)
- [How `__llm__` compilation works](docs/how_llm_compilation_works.md)
- [How to configure an LLM agent](docs/how_to_configure_an_llm_agent.md)
- [How to use driver options and caching](docs/how_to_use_driver_options_and_caching.md)
- [How the source is structured](docs/how_the_source_is_structured.md)
- [How to develop llmc++](docs/how_to_develop_llmcxx.md)
- [Prototype plan and milestones](PLAN.md)

## Author
Ivan Janevski (C) 2026
