/*
 * C++ header for llmcpp driver and generation options.
 */

#ifndef LLMCPP_DATA_GENERATION_OPTIONS_H
#define LLMCPP_DATA_GENERATION_OPTIONS_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Configures code generation, caching, diagnostics, and agent limits. */
        struct DataGenerationOptions
        {
            /** Whether rewritten source should be emitted for compilation. */
            bool m_emit_source = false;
            /** Command used to launch the LLM agent. */
            std::string m_agent_command;
            /** Whether generation must use only cached results. */
            bool m_offline = false;
            /** Whether cached results should be ignored. */
            bool m_regenerate = false;
            /** Whether generated results should be read from and written to cache. */
            bool m_use_cache = true;
            /** Directory used for generated-result cache entries. */
            std::string m_cache_dir;
            /** Whether generated bodies should be printed. */
            bool m_dump = false;
            /** Whether agent task context should be printed. */
            bool m_dump_context = false;
            /** Whether generation progress should be printed. */
            bool m_verbose = false;
            /** Whether ordinary generation progress should be suppressed. */
            bool m_quiet = false;
            /** Maximum accepted-body attempts per target. */
            unsigned m_max_attempts = 4;
            /** Maximum compiler-context tool calls per target. */
            unsigned m_max_tool_calls = 60;
            /** Generation timeout in seconds. */
            unsigned m_timeout_seconds = 600;
            /** Absolute path of the llmc++ executable. */
            std::string m_executable;
        };

    }
}

#endif
