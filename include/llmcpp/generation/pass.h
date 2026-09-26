/*
 * C++ header for coordinating one llm compilation pass.
 */

#ifndef LLMCPP_PASS_H
#define LLMCPP_PASS_H

#include "llmcpp/agent/agent_session.h"
#include "llmcpp/data/pass_result.h"
#include "llmcpp/generation/pass_state.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceLocation.h"

#include <string>
#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class ASTContext;
    class CompilerInstance;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp generation declarations. */
    namespace generation
    {
        /** Coordinates target discovery, generation, validation, and source rewriting. */
        class Pass
        {
        public:
            /**
             * Initialize a pass over one compiler invocation.
             *
             * @param ci Active compiler instance.
             * @param opts Driver and generation options.
             * @param cc1Args Original Clang frontend arguments.
             * @param originalSource Original main-source contents.
             * @param result Output pass result.
             */
            Pass(::clang::CompilerInstance &ci, const data::Options &opts,
                 std::vector<std::string> cc1Args, std::string originalSource,
                 data::PassResult &result);
            /**
             * Run generation and rewriting after parsing succeeds.
             *
             * @param ctx Parsed AST context.
             */
            void run(::clang::ASTContext &ctx);
            /**
             * Return annotation locations collected during preprocessing.
             *
             * @return Mutable annotation-location collection.
             */
            std::vector<::clang::SourceLocation> &keyword_locations();
            /**
             * Return shared state for the active compilation pass.
             *
             * @return Mutable pass state.
             */
            PassState &state();

        private:
            /**
             * Emit a pass diagnostic using a compile-time format string.
             *
             * @tparam N Format string size.
             * @param location Diagnostic source location.
             * @param level Diagnostic severity.
             * @param format Diagnostic format string.
             * @return Configured diagnostic builder.
             */
            template <unsigned N>
            ::clang::DiagnosticBuilder report(::clang::SourceLocation location,
                                              ::clang::DiagnosticsEngine::Level level,
                                              const char (&format)[N]);
            /**
             * Collect and validate AST targets associated with annotations.
             *
             * @param ctx Parsed AST context.
             */
            void collect_targets(::clang::ASTContext &ctx);
            /**
             * Validate one target and record its prompt source range.
             *
             * @param ctx Parsed AST context.
             * @param target data::Target under validation.
             * @param keyword Annotation source location.
             * @return Whether the target is valid.
             */
            bool check_target(::clang::ASTContext &ctx, data::Target &target,
                              ::clang::SourceLocation keyword);
            /**
             * Assign stable names, signatures, and cache keys to collected targets.
             *
             * @param ctx Parsed AST context.
             */
            void name_targets(::clang::ASTContext &ctx);
            /**
             * Generate and validate one target implementation.
             *
             * @param target data::Target to generate.
             * @return Whether generation succeeded.
             */
            bool generate(data::Target &target);
            /**
             * Apply generated bodies and remove annotation spellings.
             *
             * @return Rewritten source text.
             */
            std::string rewrite() const;
            /**
             * Resolve the cache directory for the active source file.
             *
             * @return Cache directory path.
             */
            std::string cache_dir() const;
            /**
             * Resolve the cache path for one target.
             *
             * @param target data::Target with a stable cache key.
             * @return Cache entry path.
             */
            std::string cache_path(const data::Target &target) const;
            /**
             * Load a compatible generated implementation from cache.
             *
             * @param target data::Target receiving cached generation metadata.
             * @return Whether a compatible entry was loaded.
             */
            bool read_cache(data::Target &target) const;
            /**
             * Store a generated implementation and metadata atomically.
             *
             * @param target Generated target to cache.
             */
            void write_cache(const data::Target &target);
            /**
             * Print a generated target implementation for diagnostics.
             *
             * @param target Generated target to print.
             */
            void dump(const data::Target &target) const;
            /**
             * Resolve an annotation source location for a target.
             *
             * @param target data::Target with an annotation offset.
             * @return Annotation source location.
             */
            ::clang::SourceLocation keyword_loc(const data::Target &target) const;
            /** Annotation locations collected during preprocessing. */
            std::vector<::clang::SourceLocation> m_keywords;
            /** Shared state for the active translation unit. */
            PassState m_state;
            /** Active compiler instance. */
            ::clang::CompilerInstance &m_ci;
            /** Immutable driver and generation options. */
            const data::Options &m_opts;
            /** Output result updated as the pass progresses. */
            data::PassResult &m_result;
            /** JSON-RPC session used to request generated code. */
            agent::AgentSession m_agent;
            /** Original Clang frontend arguments. */
            std::vector<std::string> m_cc1_args;
            /** Original main-source contents. */
            std::string m_original_source;
        };

    }
}

#endif
