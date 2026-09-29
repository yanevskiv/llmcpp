/*
 * C++ file for merging closed integer intervals.
 */

// Example type for closed ranges.
#include "include/08_interval.h"

// Standard headers for sorting, output, errors, and storage.
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

// Sort and merge closed ranges, rejecting inverted endpoints.
__llm__ std::vector<Interval> merge_intervals(std::vector<Interval> intervals)
{
    Treat intervals as closed ranges. Reject any interval whose start
    exceeds its end by throwing std::invalid_argument.
    Return intervals sorted by start, merging overlapping ranges and
    ranges that share an endpoint. Empty input produces empty output.
}

// Merge a set of ranges and print the result.
int main()
{
    const auto merged = merge_intervals({{8, 10}, {2, 5}, {1, 3}, {5, 7}, {2, 2}});
    for (const Interval &interval : merged) {
        std::cout << '[' << interval.m_start << ", " << interval.m_end << "]\n";
    }
}
