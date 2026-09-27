/*
 * C++ file for using a Python agent with a local model server.
 * Compile from the project root:
 * llmc++ -fllm-agent='python3 examples/python/example18_agent.py' \
 *        -fllm-agent-config=examples/json/example18_config.json \
 *        examples/example18_custom_agent.cpp
 */

#include <iostream>
#include <vector>

// Sort scores through the custom agent's model server.
__llm__ void sort_scores(std::vector<int> &scores)
{
    Sort scores from highest to lowest.
}

// Print the generated result.
int main()
{
    std::vector<int> scores{12, 8, 15};
    sort_scores(scores);
    for (int score : scores) {
        std::cout << score << '\n';
    }
}
