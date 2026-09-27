/*
 * C++ file for a generated projected frequency table.
 */

// Example type for people grouped by a projection.
#include "include/12_person.h"

#include <cstddef>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

// Build a projected frequency table.
template <typename Iterator, typename Projection>
__llm__ auto projected_frequency_table(Iterator begin, Iterator end, Projection project)
    -> std::map<std::decay_t<decltype(project(*begin))>, std::size_t>
{
    Return a map that counts each value produced by applying project to every
    element in the half-open iterator range [begin, end).
}

// Run the projected-frequency-table example.
int main()
{
    std::vector<Person> people{{"Ada", 36}, {"Alan", 41}, {"Grace", 36}, {"Edsger", 41}};
    auto ages = projected_frequency_table(people.begin(), people.end(),
                                          [](const Person &person) { return person.m_age; });
    for (const auto &[age, count] : ages) {
        std::cout << age << ": " << count << '\n';
    }
}
