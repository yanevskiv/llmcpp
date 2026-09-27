/*
 * C++ file for text test support.
 */

// Headers for integration-test support and its dependencies.
#include "llmcpp/test/test_text.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
// Namespace for llmcpp integration-test support.
namespace llmcpp::test
{
    // Quote one value for a POSIX shell command.
    std::string shell_quote(const std::string &value)
    {
        std::string quoted = "'";
        for (char c : value) {
            quoted += c == '\'' ? "'\\''" : std::string(1, c);
        }
        return quoted + "'";
    }

    // Read an entire test file.
    std::string read_file(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    // Count non-overlapping occurrences of a substring.
    size_t count_occurrences(const std::string &text, const std::string &needle)
    {
        size_t count = 0;
        for (size_t pos = 0; (pos = text.find(needle, pos)) != std::string::npos;
             pos += needle.size()) {
            ++count;
        }
        return count;
    }

    // Check that text contains every expected fragment.
    void check_contains(const std::string &text, std::initializer_list<const char *> needles)
    {
        INFO(text);
        for (const char *needle : needles) {
            INFO("Expected output to contain: " << needle);
            CHECK(text.find(needle) != std::string::npos);
        }
    }
}
