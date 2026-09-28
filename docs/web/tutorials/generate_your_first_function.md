# Generate your first function

Now we will write the sorting program. Create a directory for it outside the
repository so its source and cache stay separate from compiler development.
Keep using the shell configured in the previous tutorial.

Create `main.cpp` with this content:

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

__llm__ void sort_scores(std::vector<int> &scores)
{
    Sort scores from highest to lowest.
}

int main()
{
    std::vector<int> scores{12, 40, 7};
    sort_scores(scores);
    for (int score : scores) {
        std::cout << score << '\n';
    }
}
```

The includes and `main` are ordinary C++. Only `sort_scores` needs generation.
Its signature tells the agent which vector to modify; the body describes the
ordering. Including `<algorithm>` makes the standard sorting facilities available.

Compile and run it:

```sh
llmc++ main.cpp -o scores
./scores
```

You should see:

```text
40
12
7
```

The compiler generated and accepted a body before compiling the executable.
It also saved that body in `.llmcache/` beside `main.cpp`. The executable itself
contains no model call, so running it again does not generate anything.

That output is a useful first check, but three values do not prove the function
is correct. Next, [review the generated code](review_the_generated_code.md).
