/*
 * C++ file for recording and replaying generation without a model.
 * Record from the project root:
 * llmc++ -fllm-transcript=trace.jsonl examples/19_transcript.cpp
 * Replay the same task:
 * llmc++ -fllm-regenerate -fllm-agent='llmcpp-agent --replay trace.jsonl' \
 *        examples/19_transcript.cpp
 */

#include <iostream>

// Compute a triangular number.
__llm__ int triangular(int count)
{
    Return the sum of integers from one through count.
}

// Print the generated result.
int main()
{
    std::cout << triangular(5) << '\n';
}
