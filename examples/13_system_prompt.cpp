/*
 * C++ file for replacing the compiler's system instructions.
 * Compile from the project root:
 * llmc++ -fllm-system-prompt=examples/markdown/13_prompt.md \
 *        examples/13_system_prompt.cpp
 */

#include <iostream>

// Compute the absolute difference.
__llm__ int distance(int left, int right)
{
    Return the absolute difference between left and right.
}

// Print the generated result.
int main()
{
    std::cout << distance(8, 3) << '\n';
}
