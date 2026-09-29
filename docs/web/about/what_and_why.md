# What & Why

You know the function you want: its name, its parameters, and what it should
do. Usually the next step is to write the implementation. LLMCPP explores
another route: put that description in the function body and have an LLM
write the implementation during compilation.

```cpp
#include <vector>

__llm__ void sort_scores(std::vector<int> &scores)
{
    Sort scores from highest to lowest.
}
```

This is not a call to a model every time `sort_scores` runs. `llmc++` generates
the body while building your program, checks it with Clang, and compiles the
accepted C++ into the executable. Running the executable needs no model,
credentials, or generation service.

## Why put generation in the compiler?

A compiler already knows the types, declarations, members, and captures around
a function. LLMCPP gives the agent tools to ask for that context and try candidate
bodies. Instead of guessing whether a member exists or a call is well-formed,
the agent can ask Clang and correct a rejected implementation.

The interface remains ordinary C++. You can call a generated function from
hand-written code, use private member state in a method, or generate a lambda
inside an algorithm. Cached bodies let later builds reuse accepted code.

## What it does not promise

Compilation checks whether the code is valid C++, not whether it is correct.
An ambiguous instruction can produce a perfectly compilable wrong answer.
Treat the generated body as code you need to read and test, not as proof that
your specification has been satisfied.

LLMCPP is a prototype. Annotations must be in the main source file, not an
included header. Templates receive one generated body rather than one per
instantiation.

Ready to try it? [Build the compiler](../tutorials/build_the_compiler.md).
