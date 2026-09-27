/*
 * C++ file for llmc++-specific command-line option handling.
 */

// Project header for llmc++ option handling.
#include "llmcpp/driver_options.h"
#include "llmcpp/agent_prompt.h"

// LLVM header for string references.
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"

// Standard header for environment defaults.
#include <cstdlib>

// Namespace for llmc++ driver option handling.
namespace llmcpp
{

    // Initialize option handling for one llmc++ invocation.
    DriverOptions::DriverOptions(std::string executable)
    {
        m_options.m_executable = std::move(executable);
        m_options.m_system_prompt = agent_system_prompt().str();
        if (const char *backend = std::getenv("LLMCPP_BACKEND")) {
            m_options.m_backend = backend;
        }
        if (const char *model = std::getenv("LLMCPP_MODEL")) {
            m_options.m_model = model;
        }
    }

    // Parse arguments following the executable name.
    bool DriverOptions::parse(llvm::ArrayRef<const char *> args, std::string &error)
    {
        bool valid = true;
        std::vector<bool> handled;
        for (const char *arg : args) {
            handled.push_back(parse_llm_option(arg, error));
            if (handled.back() && !error.empty()) {
                valid = false;
            }
        }

        llvm::StringRef output;
        for (unsigned index = 0; index < args.size(); ++index) {
            llvm::StringRef arg = args[index];
            if (handled[index]) {
                continue;
            }
            if (arg == "-o" && index + 1 < args.size()) {
                output = args[++index];
            } else if (arg.starts_with("-o") && arg.size() > 2) {
                output = arg.drop_front(2);
            }
        }
        llvm::StringRef extension = llvm::sys::path::extension(output);
        if (extension == ".cpp" || extension == ".cc" || extension == ".cxx") {
            m_options.m_emit_source = true;
        }

        for (unsigned index = 0; index < args.size(); ++index) {
            llvm::StringRef arg = args[index];
            if (handled[index]) {
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
            if (arg == "--help" || arg == "-help" || arg == "--help-hidden") {
                m_wants_help = true;
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
        return valid && (m_wants_help || resolve_configuration(error));
    }

    // Read configuration once so every target receives the same instructions.
    bool DriverOptions::resolve_configuration(std::string &error)
    {
        llvm::StringRef backend = m_options.m_backend;
        if (!backend.empty() && backend != "anthropic" && backend != "openai" &&
            backend != "codex" && backend != "claude") {
            error = "unknown LLM backend '" + backend.str() +
                    "'; choose anthropic, openai, codex, or claude";
            return false;
        }
        std::vector<std::string> files = m_options.m_append_system_prompt_files;
        if (!m_options.m_system_prompt_file.empty()) {
            m_options.m_system_prompt.clear();
            files.insert(files.begin(), m_options.m_system_prompt_file);
        }
        for (const std::string &file : files) {
            auto buffer = llvm::MemoryBuffer::getFile(file);
            if (!buffer) {
                error = "cannot read system prompt '" + file + "': " + buffer.getError().message();
                return false;
            }
            llvm::StringRef text = (*buffer)->getBuffer();
            if (!llvm::json::isUTF8(text)) {
                error = "system prompt '" + file + "' is not UTF-8";
                return false;
            }
            if (!m_options.m_system_prompt.empty()) {
                m_options.m_system_prompt += "\n\n";
            }
            m_options.m_system_prompt += text.str();
        }
        if (!m_options.m_agent_config_file.empty()) {
            auto buffer = llvm::MemoryBuffer::getFile(m_options.m_agent_config_file);
            if (!buffer) {
                error = "cannot read agent configuration '" + m_options.m_agent_config_file + "'";
                return false;
            }
            auto value = llvm::json::parse((*buffer)->getBuffer());
            if (!value) {
                llvm::consumeError(value.takeError());
                error = "agent configuration must be a JSON object";
                return false;
            }
            if (!value->getAsObject()) {
                error = "agent configuration must be a JSON object";
                return false;
            }
            m_options.m_agent_config = (*buffer)->getBuffer().str();
        }
        return true;
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
    const data::DataGenerationOptions &DriverOptions::options() const
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

    // Report whether driver help was requested.
    bool DriverOptions::wants_help() const
    {
        return m_wants_help;
    }

    // Describe generation options alongside Clang's driver help.
    void DriverOptions::print_help(llvm::raw_ostream &stream)
    {
        stream
            << "\nLLMCPP OPTIONS:\n"
               "  --llm                          Write generated C++ source and stop\n"
               "  -o <file>.cpp|.cc|.cxx          Select generated-source mode implicitly\n"
               "  -fllm                          Accepted for compatibility; generation is "
               "enabled\n"
               "  -fllm-backend=<backend>         Select anthropic, openai, codex, or claude\n"
               "                                 Overrides LLMCPP_BACKEND\n"
               "  -fllm-agent=<command>           Run a custom external agent\n"
               "  -fllm-model=<id>                Override LLMCPP_MODEL\n"
               "  -fllm-system-prompt=<file>      Replace the built-in system prompt\n"
               "  -fllm-append-system-prompt=<file>\n"
               "                                 Append system instructions; repeatable\n"
               "  -fllm-agent-config=<file>       Pass a JSON configuration object to the agent\n"
               "  -fllm-offline                   Use cached bodies only\n"
               "  -fllm-regenerate                Ignore cached bodies and generate again\n"
               "  -fllm-no-cache                  Disable cache reads and writes\n"
               "  -fllm-cache-dir=<directory>     Override the source's .llmcache directory\n"
               "  -fllm-hash-abbrev=<n>           Minimum hash length (default: 7, maximum: 64)\n"
               "  -fllm-max-attempts=<count>      Limit rejected submissions (default: 4)\n"
               "  -fllm-max-tool-calls=<count>    Limit compiler tool calls (default: 60)\n"
               "  -fllm-timeout=<seconds>         Set the generation deadline (default: 600)\n"
               "  -fllm-dump                      Print accepted generated bodies\n"
               "  -fllm-dump-context              Print compiler context without generation\n"
               "  -fllm-verbose                   Print generation progress and agent tool logs\n"
               "  -fllm-transcript=<file>         Append a redacted JSONL generation transcript\n"
               "\nGeneration requires -fllm-backend or LLMCPP_BACKEND, unless a custom agent\n"
               "is supplied. Successful compilation is silent by default.\n";
    }

    // Parse one llmc++-specific argument.
    bool DriverOptions::parse_llm_option(llvm::StringRef arg, std::string &error)
    {
        if (arg == "--llm") {
            m_options.m_emit_source = true;
        } else if (arg == "-fllm") {
        } else if (arg.consume_front("-fllm-backend=")) {
            m_options.m_backend = arg.str();
            if (arg.empty()) {
                error = "-fllm-backend requires a backend";
            }
        } else if (arg.consume_front("-fllm-agent=")) {
            m_options.m_agent_command = arg.str();
        } else if (arg.consume_front("-fllm-system-prompt=")) {
            m_options.m_system_prompt_file = arg.str();
            if (arg.empty()) {
                error = "-fllm-system-prompt requires a file";
            }
        } else if (arg.consume_front("-fllm-append-system-prompt=")) {
            m_options.m_append_system_prompt_files.push_back(arg.str());
            if (arg.empty()) {
                error = "-fllm-append-system-prompt requires a file";
            }
        } else if (arg.consume_front("-fllm-model=")) {
            m_options.m_model = arg.str();
            if (arg.empty()) {
                error = "-fllm-model requires a model id";
            }
        } else if (arg.consume_front("-fllm-agent-config=")) {
            m_options.m_agent_config_file = arg.str();
            if (arg.empty()) {
                error = "-fllm-agent-config requires a file";
            }
        } else if (arg.consume_front("-fllm-transcript=")) {
            m_options.m_transcript_file = arg.str();
            if (arg.empty()) {
                error = "-fllm-transcript requires a file";
            }
        } else if (arg == "-fllm-offline") {
            m_options.m_offline = true;
        } else if (arg == "-fllm-regenerate") {
            m_options.m_regenerate = true;
        } else if (arg == "-fllm-no-cache") {
            m_options.m_use_cache = false;
        } else if (arg.consume_front("-fllm-cache-dir=")) {
            m_options.m_cache_dir = arg.str();
        } else if (arg.consume_front("-fllm-hash-abbrev=")) {
            if (!parse_unsigned(arg, m_options.m_hash_abbrev) || m_options.m_hash_abbrev > 64) {
                error = "invalid value for -fllm-hash-abbrev (expected 1 through 64)";
            }
        } else if (arg == "-fllm-dump") {
            m_options.m_dump = true;
        } else if (arg == "-fllm-dump-context") {
            m_options.m_dump_context = true;
        } else if (arg == "-fllm-verbose") {
            m_options.m_verbose = true;
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
        return !value.getAsInteger(10, out) && out != 0;
    }

}
