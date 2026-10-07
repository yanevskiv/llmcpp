#include "include/04_options_context.h"

// Override generation limits, model, and cache salt for this target.
__llm__(max_attempts(12), timeout(120), model("target-model"),
        cache_salt("reviewed")) int limited()
{
    Return the configured score.
}

// Request fresh generation for the second target.
__llm__(no_cache) int fresh()
{
    Return the configured score.
}
