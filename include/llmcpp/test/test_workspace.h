/*
 * C++ header for workspace test support.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_TEST_WORKSPACE_H
#define LLMCPP_TEST_WORKSPACE_H

#include "llmcpp/test/test_command_result.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>
/** Namespace for llmcpp integration-test support. */
namespace llmcpp::test
{
    /** Class for isolated fixture commands. */
    class TestWorkspace
    {
    public:
        /** Initialize the isolated workspace. */
        TestWorkspace();
        /** Stop and clean up the isolated workspace. */
        ~TestWorkspace();
        /**
         * Get the workspace directory.
         * @return Isolated fixture directory.
         */
        const std::filesystem::path &path() const;
        /**
         * Run a child command.
         * @param executable Program to execute.
         * @param arguments Command-line arguments.
         * @param environment Environment overrides.
         * @return Exit status and captured output.
         */
        TestCommandResult
        run(const std::string &executable, const std::vector<std::string> &arguments = {},
            const std::vector<std::pair<std::string, std::string>> &environment = {});
        /**
         * Run llmc++.
         * @param arguments Compiler arguments.
         * @param environment Environment overrides.
         * @return Exit status and captured output.
         */
        TestCommandResult
        llmcpp(const std::vector<std::string> &arguments,
               const std::vector<std::pair<std::string, std::string>> &environment = {});
        /**
         * Run llmc++ with the scripted agent.
         * @param script Relative JSON script path.
         * @param arguments Compiler arguments.
         * @return Exit status and captured output.
         */
        TestCommandResult mock(const std::string &script, std::vector<std::string> arguments);

    private:
        /** Isolated fixture directory. */
        std::filesystem::path m_root;
        /** Sequence number for command output files. */
        unsigned m_command = 0;
    };
}

#endif
