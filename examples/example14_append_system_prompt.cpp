/*
 * C++ file for adding project rules to the compiler's instructions.
 * Compile from the project root:
 * llmc++ -fllm-append-system-prompt=examples/markdown/example14_rules.md \
 *        examples/example14_append_system_prompt.cpp
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
