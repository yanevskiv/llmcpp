/*
 * C++ header for command result test support.
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

#ifndef LLMCPP_TEST_COMMAND_RESULT_H
#define LLMCPP_TEST_COMMAND_RESULT_H

#include <string>
/** Namespace for llmcpp integration-test support. */
namespace llmcpp::test
{
    /** Structure for child command status and output. */
    struct TestCommandResult
    {
        /** Child process exit status. */
        int m_status;
        /** Captured standard output. */
        std::string m_out;
        /** Captured standard error. */
        std::string m_err;
    };
}

#endif
