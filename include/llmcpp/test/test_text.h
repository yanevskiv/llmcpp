/*
 * C++ header for text test support.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_TEST_TEXT_H
#define LLMCPP_TEST_TEXT_H

#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <string>
/** Namespace for llmcpp integration-test support. */
namespace llmcpp::test
{
    /**
     * Quote a POSIX shell argument.
     * @param value Argument to quote.
     * @return Shell-safe quoted argument.
     */
    std::string shell_quote(const std::string &value);
    /**
     * Read a file.
     * @param path File to read.
     * @return File contents.
     */
    std::string read_file(const std::filesystem::path &path);
    /**
     * Count non-overlapping occurrences.
     * @param text Text to search.
     * @param needle Substring to count.
     * @return Match count.
     */
    size_t count_occurrences(const std::string &text, const std::string &needle);
    /**
     * Check required text fragments.
     * @param text Text to inspect.
     * @param needles Required fragments.
     */
    void check_contains(const std::string &text, std::initializer_list<const char *> needles);

}

#endif
