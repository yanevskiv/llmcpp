#include "include/04_options_context.h"

// Accept a bare modifier.
__llm__ int bare()
{
    Return the score.
}

// Accept an empty modifier list.
__llm__() int empty()
{
    Return the score.
}

// Apply function-level options.
__llm__(max_attempts(2)) int configured()
{
    Return the score.
}
