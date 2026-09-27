/*
 * C++ header for command result test support.
 */

#ifndef LLMCPP_TEST_COMMAND_RESULT_H
#define LLMCPP_TEST_COMMAND_RESULT_H

#include <string>
/** Namespace for llmcpp integration-test support. */
namespace llmcpp::test
{
    /** Structure for child command status and output. */
    struct CommandResult
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
