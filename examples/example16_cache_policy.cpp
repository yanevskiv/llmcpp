/*
 * C++ file for selecting cache policy separately for each function.
 */

#include <iostream>
#include <vector>

// Regenerate a body on each compilation.
__llm__(no_cache) int lowest(const std::vector<int> &scores)
{
    Return the lowest score, or zero if scores is empty.
}

// Keep an independently named cache policy for a reviewed implementation.
__llm__(cache("scores-v1")) int highest(const std::vector<int> &scores)
{
    Return the highest score, or zero if scores is empty.
}

// Print both generated results.
int main()
{
    std::vector<int> scores{12, 8, 15};
    std::cout << lowest(scores) << ' ' << highest(scores) << '\n';
}
