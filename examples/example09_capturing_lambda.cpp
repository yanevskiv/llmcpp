/*
 * C++ file for a generated capturing lambda.
 */

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

// Filter numbers with a generated lambda that captures the threshold.
int main()
{
    const std::vector<int> values{2, 7, 4, 9, 1};
    const int minimum = 5;
    auto meetsMinimum = __llm__ [minimum](int value) -> bool {
        Return whether value is at least minimum.
    };

    std::vector<int> selected;
    std::copy_if(values.begin(), values.end(), std::back_inserter(selected), meetsMinimum);
    for (int value : selected) {
        std::cout << value << '\n';
    }
}
