# Review the generated code

The program runs, but we have not yet read the implementation. In the same
directory, ask the compiler to write the rewritten source rather than build it:

```sh
llmc++ --llm -fllm-offline main.cpp
```

Open `main.llm.cpp`. The `__llm__` modifier and prose body have been replaced
with ordinary C++. Offline mode ensures this command inspects the cached body
from the previous tutorial rather than asking for a different implementation.

For our sorting function, look for descending order, in-place modification,
and no assumptions about the vector being nonempty. Check repeated and negative
values too. The exact implementation can vary between models or generations.

You can compile the rewritten file with a conventional C++ compiler:

```sh
g++ -std=c++17 main.llm.cpp -o reviewed_scores
./reviewed_scores
```

This separates two questions: does the body compile, and does it do what we
asked? The compiler helps with the first; review and behavioral tests address
the second. Keep the reviewed source if you want an ordinary C++ artifact
independent of the generation workflow.

Now [build without a model](build_without_a_model.md) using the accepted cache.
