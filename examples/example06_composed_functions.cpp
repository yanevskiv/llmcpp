/*
 * C++ file for composed generated functions.
 */

#include <cctype>
#include <iostream>
#include <string>

// Normalize a human-readable label.
__llm__ std::string normalize_label(const std::string &label)
{
    Trim surrounding whitespace, lowercase ASCII letters, and collapse every
    run of internal whitespace, underscores, or hyphens to one hyphen.
}

// Compare two normalized labels.
__llm__ bool same_label(const std::string &left, const std::string &right)
{
    Return whether normalize_label(left) equals normalize_label(right).
}

// Run the composed-functions example.
int main()
{
    std::cout << std::boolalpha << same_label("Release_Candidate", "release candidate") << '\n';
}
