/*
 * C++ file for appending project rules to the compiler's instructions.
 * Compile from the project root:
 * llmc++ -fllm-append-prompt=examples/markdown/14_rules.md \
 *        examples/14_append_prompt.cpp
 */

#include <iostream>
#include <vector>

// Find the total score.
__llm__ int total(const std::vector<int> &scores)
{
    Return the sum of scores.
}

// Print the generated result.
int main()
{
    std::cout << total({12, 8, 15}) << '\n';
}
