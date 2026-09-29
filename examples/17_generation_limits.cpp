/*
 * C++ file for giving one function a smaller generation budget.
 * Inspect the effective policy with -fllm-dump-context.
 */

#include <iostream>
#include <vector>

// Count passing scores within the requested generation budget.
__llm__(max_attempts(2), timeout(120)) int passing(const std::vector<int> &scores)
{
    Count scores greater than or equal to 60.
}

// Print the generated result.
int main()
{
    std::cout << passing({45, 60, 82}) << '\n';
}
