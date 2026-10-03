/*
 * C++ header for llmc++-specific command-line option handling.
 */

#ifndef LLMCPP_DRIVER_OPTIONS_H
#define LLMCPP_DRIVER_OPTIONS_H

#include "llmcpp/data/data_generation_options.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Separates llmc++ options from arguments forwarded to the Clang driver. */
    class DriverOptions
    {
    public:
        /**
         * Initialize option handling for one llmc++ invocation.
         *
         * @param executable Absolute path of the llmc++ executable.
         */
        explicit DriverOptions(std::string executable);
        /**
         * Parse arguments following the executable name.
         *
         * @param args Raw command-line arguments.
         * @param error Destination for an option-parsing diagnostic.
         * @return Whether every llmc++ option was valid.
         */
        bool parse(llvm::ArrayRef<const char *> args, std::string &error);
        /**
         * Build arguments for Clang's driver.
         *
         * @param argv0 Original executable spelling.
         * @param resourceDir Clang resource directory to supply when absent.
         * @return Argument pointers valid while this object and resourceDir live.
         */
        std::vector<const char *> driver_arguments(const char *argv0,
                                                   llvm::StringRef resourceDir) const;
        /**
         * Return llmc++ generation options.
         *
         * @return Immutable generation options.
         */
        const data::DataGenerationOptions &options() const;
        /**
         * Return the requested generated-source output path.
         *
         * @return Empty when the default output name should be used.
         */
        const std::string &output_path() const;
        /**
         * Report whether the forwarded invocation requests preprocessing.
         *
         * @return True when `-E` was supplied.
         */
        bool wants_preprocess() const;
        /**
         * Report whether driver help was requested.
         * @return True when a help option was supplied.
         */
        bool wants_help() const;
        /**
         * Print options handled by llmc++ rather than Clang.
         * @param stream Destination for command-line help.
         */
        static void print_help(llvm::raw_ostream &stream);

    private:
        /**
         * Load environment defaults before command-line overrides.
         * @param error Destination for invalid environment diagnostics.
         * @return Whether every supplied default was valid.
         */
        bool parse_environment(std::string &error);
        /**
         * Parse a boolean option with an optional explicit value.
         * @param arg Argument to inspect.
         * @param error Destination for invalid boolean diagnostics.
         * @return Whether the argument names a boolean option.
         */
        bool parse_boolean_option(llvm::StringRef arg, std::string &error);
        /**
         * Resolve prompt files and agent configuration.
         * @param error Destination for file and JSON diagnostics.
         * @return Whether configuration was valid.
         */
        bool resolve_configuration(std::string &error);
        /**
         * Parse one llmc++-specific argument.
         *
         * @param arg Argument to inspect.
         * @param error Destination for an option-parsing diagnostic.
         * @return True when arg was an llmc++ option.
         */
        bool parse_llm_option(llvm::StringRef arg, std::string &error);
        /**
         * Parse a decimal unsigned option value.
         *
         * @param value Text to parse.
         * @param out Destination for the parsed value.
         * @return True when value is a valid decimal unsigned integer.
         */
        static bool parse_unsigned(llvm::StringRef value, unsigned &out);
        /** Options that control generation and agent behavior. */
        data::DataGenerationOptions m_options;
        /** Arguments forwarded unchanged to Clang's driver. */
        std::vector<std::string> m_clang_args;
        /** Explicit output path used by generated-source mode. */
        std::string m_output_path;
        /** Whether the forwarded invocation requests preprocessing. */
        bool m_wants_preprocess = false;
        /** Whether the caller requested driver help. */
        bool m_wants_help = false;
        /** Whether the caller explicitly selected a Clang driver mode. */
        bool m_has_driver_mode = false;
        /** Whether the caller explicitly selected a Clang resource directory. */
        bool m_has_resource_dir = false;
    };

}

#endif
