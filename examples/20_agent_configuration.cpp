/*
 * C++ file for configuring the bundled Python agent explicitly.
 * Compile from the project root:
 * llmc++ -fllm-agent-config=examples/json/20_config.json \
 *        examples/20_agent_configuration.cpp
 */

#include <iostream>
#include <vector>

// Count even values using the configured backend.
__llm__ int count_even(const std::vector<int> &values)
{
    Count the even values.
}

// Print the generated result.
int main()
{
    std::cout << count_even({1, 2, 3, 4}) << '\n';
}
