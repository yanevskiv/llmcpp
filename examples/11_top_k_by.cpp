/*
 * C++ file for a generated STL top-k helper.
 */

// Example type for tasks ranked by a projection.
#include "include/11_task.h"

#include <cstddef>
#include <iostream>
#include <vector>

// Select the greatest projected values.
template <typename Range, typename Projection>
__llm__ auto top_k_by(const Range &values, std::size_t count, Projection project)
    -> std::vector<typename Range::value_type>
{
    Return at most count elements from values with the greatest project value.
    Order results from greatest to least, preserving input order for ties.
}

// Run the STL top-k example.
int main()
{
    std::vector<RankedTask> tasks{{"Write documentation", 2},
                                  {"Fix regression", 5},
                                  {"Review patch", 5},
                                  {"Refactor parser", 3}};
    for (const RankedTask &task :
         top_k_by(tasks, 3, [](const RankedTask &value) { return value.m_priority; })) {
        std::cout << task.m_priority << ": " << task.m_name << '\n';
    }
}
