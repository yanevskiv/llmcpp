/*
 * C++ header for shadow-compiler-backed type inspection tools.
 */

#ifndef LLMCPP_COMPILER_TYPE_INSPECTOR_H
#define LLMCPP_COMPILER_TYPE_INSPECTOR_H

#include "llmcpp/data/data_tool_result.h"
#include "llmcpp/generation_context.h"

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
    /** Implements type inspection tools through isolated shadow compilations. */
    class CompilerTypeInspector
    {
    public:
        /**
         * Create type tools for one generation target.
         *
         * @param state Shared translation-unit state.
         * @param t data::DataGenerationTarget whose lexical context should be used.
         */
        CompilerTypeInspector(GenerationContext &state, data::DataGenerationTarget &t);
        /**
         * Describe the properties of a type visible at the target.
         *
         * @param type Source spelling of the type.
         * @return Tool response containing structured type information.
         */
        data::DataToolResult describe_type(llvm::StringRef type);
        /**
         * List the members visible on a record type.
         *
         * @param type Source spelling of the record type.
         * @return Tool response containing member declarations.
         */
        data::DataToolResult list_members(llvm::StringRef type);

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
        data::DataToolResult with_probe_type(llvm::StringRef type, ProbeFn fn);
        /** Shared state for the active translation unit. */
        GenerationContext &m_state;
        /** data::DataGenerationTarget whose lexical context is inspected. */
        data::DataGenerationTarget &m_target;
    };

}

#endif
