/*
 * C++ header for shadow-compiler-backed type inspection tools.
 */

#ifndef LLMCPP_TYPE_TOOLS_H
#define LLMCPP_TYPE_TOOLS_H

#include "llmcpp/data/tool_result.h"
#include "llmcpp/generation/pass_state.h"

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/StringRef.h"

#include <string>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class ASTContext;
    class Sema;
    class TypeAliasDecl;
    class QualType;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp compiler declarations. */
    namespace compiler
    {
        /** Implements type inspection tools through isolated shadow compilations. */
        class TypeTools
        {
        public:
            /**
             * Create type tools for one generation target.
             *
             * @param state Shared translation-unit state.
             * @param t data::Target whose lexical context should be used.
             */
            TypeTools(generation::PassState &state, data::Target &t);
            /**
             * Describe the properties of a type visible at the target.
             *
             * @param type Source spelling of the type.
             * @return Tool response containing structured type information.
             */
            data::ToolResult describe_type(llvm::StringRef type);
            /**
             * List the members visible on a record type.
             *
             * @param type Source spelling of the record type.
             * @return Tool response containing member declarations.
             */
            data::ToolResult list_members(llvm::StringRef type);

        private:
            using ProbeFn =
                llvm::function_ref<std::string(::clang::ASTContext &, ::clang::Sema &,
                                               ::clang::QualType, const ::clang::TypeAliasDecl *)>;
            /**
             * Resolve a source-level type and run an inspection callback.
             *
             * @param type Source spelling of the type.
             * @param fn Callback invoked with the resolved type.
             * @return Tool response produced by the callback or compiler diagnostics.
             */
            data::ToolResult with_probe_type(llvm::StringRef type, ProbeFn fn);
            /** Shared state for the active translation unit. */
            generation::PassState &m_state;
            /** data::Target whose lexical context is inspected. */
            data::Target &m_target;
        };

    }
}

#endif
