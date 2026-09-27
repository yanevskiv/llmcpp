#include "include/test_context.h"

__llm__(max_attempts(12), timeout(120), model("target-model"), cache("reviewed")) int limited()
{
    Return the configured score.
}

__llm__(no_cache) int fresh()
{
    Return the configured score.
}
