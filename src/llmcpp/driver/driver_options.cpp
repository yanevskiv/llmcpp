/*
 * C++ file for llmc++-specific command-line option handling.
 */

// Project header for llmc++ option handling.
#include "llmcpp/driver/driver_options.h"

// LLVM header for string references.
#include "llvm/ADT/StringRef.h"

// Namespace for llmc++ driver option handling.
namespace llmcpp
{
    // Namespace for llmcpp driver implementation.
    namespace driver
    {

        // Initialize option handling for one llmc++ invocation.
        DriverOptions::DriverOptions(std::string executable)
        {
            m_options.m_executable = std::move(executable);
        }

        // Parse arguments following the executable name.
        bool DriverOptions::parse(llvm::ArrayRef<const char *> args, std::string &error)
        {
            bool valid = true;
            for (const char *arg : args) {
                if (parse_llm_option(arg, error) && !error.empty()) {
                    valid = false;
                }
            }

            for (unsigned index = 0; index < args.size(); ++index) {
                llvm::StringRef arg = args[index];
                std::string ignoredError;
                if (parse_llm_option(arg, ignoredError)) {
                    continue;
                }
                if (arg.starts_with("--driver-mode=")) {
                    m_has_driver_mode = true;
                }
                if (arg == "-resource-dir" || arg.starts_with("-resource-dir=")) {
                    m_has_resource_dir = true;
                }
                if (arg == "-E") {
                    m_wants_preprocess = true;
                }
                if (m_options.m_emit_source) {
                    if (arg == "-E") {
                        continue;
                    }
                    if (arg == "-o" && index + 1 < args.size()) {
                        m_output_path = args[++index];
                        continue;
                    }
                    if (arg.starts_with("-o") && arg.size() > 2) {
                        m_output_path = arg.drop_front(2);
                        continue;
                    }
                }
                m_clang_args.push_back(arg.str());
            }
            return valid;
        }

        // Build arguments for Clang's driver.
        std::vector<const char *> DriverOptions::driver_arguments(const char *argv0,
                                                                  llvm::StringRef resourceDir) const
        {
            std::vector<const char *> args;
            args.push_back(argv0);
            if (!m_has_driver_mode) {
                args.push_back("--driver-mode=g++");
            }
            if (!m_has_resource_dir) {
                args.push_back("-resource-dir");
                args.push_back(resourceDir.data());
            }
            for (const std::string &arg : m_clang_args) {
                args.push_back(arg.c_str());
            }
            if (m_options.m_emit_source) {
                args.push_back("-fsyntax-only");
                args.push_back("-Qunused-arguments");
            } else if (m_wants_preprocess) {
                args.push_back("-C");
            }
            return args;
        }

        // Return llmc++ generation options.
        const data::Options &DriverOptions::options() const
        {
            return m_options;
        }

        // Return the requested generated-source output path.
        const std::string &DriverOptions::output_path() const
        {
            return m_output_path;
        }

        // Report whether the forwarded invocation requests preprocessing.
        bool DriverOptions::wants_preprocess() const
        {
            return m_wants_preprocess;
        }

        // Parse one llmc++-specific argument.
        bool DriverOptions::parse_llm_option(llvm::StringRef arg, std::string &error)
        {
            if (arg == "--llm") {
                m_options.m_emit_source = true;
            } else if (arg == "-fllm") {
            } else if (arg.consume_front("-fllm-agent=")) {
                m_options.m_agent_command = arg.str();
            } else if (arg == "-fllm-offline") {
                m_options.m_offline = true;
            } else if (arg == "-fllm-regenerate") {
                m_options.m_regenerate = true;
            } else if (arg == "-fno-llm-cache") {
                m_options.m_use_cache = false;
            } else if (arg.consume_front("-fllm-cache-dir=")) {
                m_options.m_cache_dir = arg.str();
            } else if (arg == "-fllm-dump") {
                m_options.m_dump = true;
            } else if (arg == "-fllm-dump-context") {
                m_options.m_dump_context = true;
            } else if (arg == "-fllm-verbose") {
                m_options.m_verbose = true;
            } else if (arg == "-fllm-quiet") {
                m_options.m_quiet = true;
            } else if (arg.consume_front("-fllm-max-attempts=")) {
                if (!parse_unsigned(arg, m_options.m_max_attempts)) {
                    error = "invalid value for -fllm-max-attempts";
                }
            } else if (arg.consume_front("-fllm-max-tool-calls=")) {
                if (!parse_unsigned(arg, m_options.m_max_tool_calls)) {
                    error = "invalid value for -fllm-max-tool-calls";
                }
            } else if (arg.consume_front("-fllm-timeout=")) {
                if (!parse_unsigned(arg, m_options.m_timeout_seconds)) {
                    error = "invalid value for -fllm-timeout";
                }
            } else {
                return false;
            }
            return true;
        }

        // Parse a decimal unsigned option value.
        bool DriverOptions::parse_unsigned(llvm::StringRef value, unsigned &out)
        {
            return !value.getAsInteger(10, out);
        }

    }
}
