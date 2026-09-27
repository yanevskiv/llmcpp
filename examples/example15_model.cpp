/*
 * C++ file for choosing a model for one function.
 * Run with an OpenAI agent; the modifier overrides -fllm-model and LLMCPP_MODEL.
 */

#include <iostream>
#include <vector>

// Find the highest score using the requested model.
__llm__(model("gpt-6-astra")) int highest(const std::vector<int> &scores)
{
    Return the highest score, or zero if scores is empty.
}

// Print the generated result.
int main()
{
    std::cout << highest({12, 8, 15}) << '\n';
}
