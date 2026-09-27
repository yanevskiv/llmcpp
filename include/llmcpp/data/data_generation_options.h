/*
 * C++ header for llmcpp driver and generation options.
 */

#ifndef LLMCPP_DATA_GENERATION_OPTIONS_H
#define LLMCPP_DATA_GENERATION_OPTIONS_H

#include <string>
#include <vector>
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
            /** Explicit backend selected by the command line or environment. */
            std::string m_backend;
            /** Command used to launch the LLM agent. */
            std::string m_agent_command;
            /** Resolved system instructions sent to every backend. */
            std::string m_system_prompt;
            /** File replacing the built-in system instructions. */
            std::string m_system_prompt_file;
            /** Files appended to the resolved system instructions, in order. */
            std::vector<std::string> m_append_prompt_files;
            /** UTF-8 reference files attached to every function task, in order. */
            std::vector<std::string> m_context_files;
            /** Resolved reference contents, parallel to the file names. */
            std::vector<std::string> m_context_contents;
            /** Requested model, overriding the agent's default. */
            std::string m_model;
            /** Agent-specific JSON configuration file. */
            std::string m_agent_config_file;
            /** Resolved agent-specific JSON object. */
            std::string m_agent_config = "{}";
            /** Optional destination for redacted generation transcripts. */
            std::string m_transcript_file;
            /** Whether generation must use only cached results. */
            bool m_offline = false;
            /** Whether cached results should be ignored. */
            bool m_regenerate = false;
            /** Whether generated results should be read from and written to cache. */
            bool m_use_cache = true;
            /** Whether cache writes are prohibited without disabling reads. */
            bool m_cache_read_only = false;
            /** Whether cache decisions should be explained on stderr. */
            bool m_explain_cache = false;
            /** Directory used for generated-result cache entries. */
            std::string m_cache_dir;
            /** Default salt added to each function's computed cache key. */
            std::string m_cache_salt;
            /** Maximum cache file age in seconds, or zero for no expiry. */
            unsigned m_cache_lifetime = 0;
            /** Minimum number of hexadecimal characters in displayed hashes and cache filenames. */
            unsigned m_hash_abbrev = 7;
            /** Whether generated bodies should be printed. */
            bool m_dump_code = false;
            /** Whether agent task context should be printed. */
            bool m_dump_context = false;
            /** Whether generation progress should be printed. */
            bool m_verbose = false;
            /** Maximum accepted-body attempts per target. */
            unsigned m_max_attempts = 4;
            /** Maximum compiler-context tool calls per target. */
            unsigned m_max_tool_calls = 60;
            /** Explicit output-token limit per model response, or zero for backend defaults. */
            unsigned m_max_output_tokens = 0;
            /** Generation timeout in seconds. */
            unsigned m_timeout_seconds = 600;
            /** Absolute path of the llmc++ executable. */
            std::string m_executable;
        };

    }
}

#endif
