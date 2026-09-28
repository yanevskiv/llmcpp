/*
 * C++ header for workspace test support.
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
