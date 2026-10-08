/*
 * C++ file for llmc++-specific command-line option handling.
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
    }

    // Parse arguments following the executable name.
    bool DriverOptions::parse(llvm::ArrayRef<const char *> args, std::string &error)
    {
        for (llvm::StringRef arg : args) {
            if (arg == "--help" || arg == "-help" || arg == "--help-hidden") {
                m_wants_help = true;
            }
        }
        bool valid = m_wants_help || parse_environment(error);
        if (!valid) {
            return false;
        }
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
        for (const std::string &file : m_options.m_context_files) {
            auto buffer = llvm::MemoryBuffer::getFile(file);
            if (!buffer) {
                error = "cannot read context file '" + file + "': " + buffer.getError().message();
                return false;
            }
            if (!llvm::json::isUTF8((*buffer)->getBuffer())) {
                error = "context file '" + file + "' is not UTF-8";
                return false;
            }
            m_options.m_context_contents.push_back((*buffer)->getBuffer().str());
        }
        std::vector<std::string> files = m_options.m_append_prompt_files;
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
               "  -fllm-append-prompt=<file>\n"
               "                                 Append system instructions; repeatable\n"
               "  -fllm-agent-config=<file>       Pass a JSON configuration object to the agent\n"
               "  -fllm-context=<file>            Attach UTF-8 reference material; repeatable\n"
               "  -fllm-cache-read-only           Read cache without writing generated bodies\n"
               "  -fllm-explain-cache             Explain cache decisions on stderr\n"
               "  -fllm-max-output-tokens=<n>     Limit tokens per model response\n"
               "  -fllm-offline                   Use cached bodies only\n"
               "  -fllm-regenerate                Generate fresh bodies, overriding offline mode\n"
               "  -fllm-no-cache                  Disable cache reads and writes\n"
               "  -fllm-cache-dir=<directory>     Override the source's .llmcache directory\n"
               "  -fllm-cache-salt=<salt>         Add a default salt to computed cache keys\n"
               "  -fllm-cache-lifetime=<seconds> Limit cache age (default: 0, no expiry)\n"
               "  -fllm-hash-abbrev=<n>           Minimum hash length (default: 7, maximum: 64)\n"
               "  -fllm-max-attempts=<count>      Limit rejected submissions (default: 4)\n"
               "  -fllm-max-tool-calls=<count>    Limit compiler tool calls (default: 60)\n"
               "  -fllm-timeout=<seconds>         Set the generation deadline (default: 600)\n"
               "  -fllm-dump-code                 Print accepted generated bodies\n"
               "  -fllm-dump-context              Print compiler context without generation\n"
               "  -fllm-verbose                   Print generation progress and agent tool logs\n"
               "  -fllm-transcript=<file>         Append a redacted JSONL generation transcript\n"
               "\nOptions use matching LLMCPP_UPPERCASE_NAMES as environment defaults.\n"
               "Boolean switches accept =true or =false (also 1/0, yes/no, on/off).\n"
               "CONTEXT and APPEND_PROMPT accept a path or JSON array of paths.\n"
               "\nGeneration requires -fllm-backend or LLMCPP_BACKEND, unless a custom agent\n"
               "is supplied. Successful compilation is silent by default.\n";
    }

    // Load environment defaults through the command-line validators.
    bool DriverOptions::parse_environment(std::string &error)
    {
        for (llvm::StringRef suffix : {"BACKEND",           "AGENT",           "MODEL",
                                       "SYSTEM_PROMPT",     "APPEND_PROMPT",   "AGENT_CONFIG",
                                       "CONTEXT",           "OFFLINE",         "REGENERATE",
                                       "NO_CACHE",          "CACHE_READ_ONLY", "EXPLAIN_CACHE",
                                       "CACHE_DIR",         "CACHE_SALT",      "CACHE_LIFETIME",
                                       "HASH_ABBREV",       "MAX_ATTEMPTS",    "MAX_TOOL_CALLS",
                                       "MAX_OUTPUT_TOKENS", "TIMEOUT",         "DUMP_CODE",
                                       "DUMP_CONTEXT",      "VERBOSE",         "TRANSCRIPT"}) {
            std::string name = "LLMCPP_" + suffix.str();
            const char *raw = std::getenv(name.c_str());
            if (!raw || !*raw) {
                continue;
            }
            std::string option = "-fllm-" + suffix.lower();
            for (char &character : option) {
                if (character == '_') {
                    character = '-';
                }
            }
            llvm::StringRef value(raw);
            std::vector<std::string> values{value.str()};
            if ((suffix == "CONTEXT" || suffix == "APPEND_PROMPT") &&
                value.ltrim().starts_with("[")) {
                auto parsed = llvm::json::parse(value);
                if (!parsed) {
                    error = name + ": " + llvm::toString(parsed.takeError());
                    return false;
                }
                auto *array = parsed->getAsArray();
                if (!array) {
                    error = name + ": expected a JSON array of file paths";
                    return false;
                }
                values.clear();
                for (const llvm::json::Value &entry : *array) {
                    auto path = entry.getAsString();
                    if (!path || path->empty()) {
                        error = name + ": expected nonempty file paths";
                        return false;
                    }
                    values.push_back(path->str());
                }
            }
            for (const std::string &entry : values) {
                parse_llm_option(option + "=" + entry, error);
                if (!error.empty()) {
                    error = name + ": " + error;
                    return false;
                }
            }
        }
        return true;
    }

    // Parse boolean switches consistently for environment and command-line values.
    bool DriverOptions::parse_boolean_option(llvm::StringRef arg, std::string &error)
    {
        auto [name, value] = arg.split('=');
        for (const auto &[option, destination] :
             {std::pair{"-fllm-offline", &m_options.m_offline},
              std::pair{"-fllm-regenerate", &m_options.m_regenerate},
              std::pair{"-fllm-no-cache", &m_options.m_use_cache},
              std::pair{"-fllm-cache-read-only", &m_options.m_cache_read_only},
              std::pair{"-fllm-explain-cache", &m_options.m_explain_cache},
              std::pair{"-fllm-dump-code", &m_options.m_dump_code},
              std::pair{"-fllm-dump-context", &m_options.m_dump_context},
              std::pair{"-fllm-verbose", &m_options.m_verbose}}) {
            if (name != option) {
                continue;
            }
            std::string normalized = value.lower();
            bool enabled = !arg.contains('=') || normalized == "1" || normalized == "true" ||
                           normalized == "yes" || normalized == "on";
            if (!enabled && normalized != "0" && normalized != "false" && normalized != "no" &&
                normalized != "off") {
                error = "invalid boolean value for " + name.str();
            } else {
                *destination = name == "-fllm-no-cache" ? !enabled : enabled;
            }
            return true;
        }
        return false;
    }

    // Parse one llmc++-specific argument.
    bool DriverOptions::parse_llm_option(llvm::StringRef arg, std::string &error)
    {
        if (parse_boolean_option(arg, error)) {
            return true;
        } else if (arg == "--llm") {
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
        } else if (arg.consume_front("-fllm-append-prompt=")) {
            m_options.m_append_prompt_files.push_back(arg.str());
            if (arg.empty()) {
                error = "-fllm-append-prompt requires a file";
            }
        } else if (arg.consume_front("-fllm-context=")) {
            m_options.m_context_files.push_back(arg.str());
            if (arg.empty()) {
                error = "-fllm-context requires a file";
            }
        } else if (arg.consume_front("-fllm-max-output-tokens=")) {
            if (!parse_unsigned(arg, m_options.m_max_output_tokens)) {
                error = "invalid value for -fllm-max-output-tokens";
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
        } else if (arg.consume_front("-fllm-cache-dir=")) {
            m_options.m_cache_dir = arg.str();
        } else if (arg.consume_front("-fllm-cache-salt=")) {
            m_options.m_cache_salt = arg.str();
            if (arg.empty()) {
                error = "-fllm-cache-salt requires a nonempty salt";
            }
        } else if (arg.consume_front("-fllm-cache-lifetime=")) {
            if (arg.getAsInteger(10, m_options.m_cache_lifetime)) {
                error = "invalid value for -fllm-cache-lifetime (expected a nonnegative integer)";
            }
        } else if (arg.consume_front("-fllm-hash-abbrev=")) {
            if (!parse_unsigned(arg, m_options.m_hash_abbrev) || m_options.m_hash_abbrev > 64) {
                error = "invalid value for -fllm-hash-abbrev (expected 1 through 64)";
            }
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
