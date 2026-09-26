/*
 * C++ header for the compiler-context tool server for one generation target.
 */

#ifndef LLMCPP_TOOL_SERVER_H
#define LLMCPP_TOOL_SERVER_H

#include "llmcpp/agent/tool_handler.h"
#include "llmcpp/generation/pass_state.h"

#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/Support/JSON.h"

#include <string>
#include <vector>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompilerInstance;
    class Sema;
}
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp agent declarations. */
    namespace agent
    {
        /** Answers compiler-context tool calls for one generation target. */
        class ToolServer : public ToolHandler
        {
        public:
            /**
             * Create a tool server for a target.
             *
             * @param state Shared translation-unit state.
             * @param t data::Target being generated.
             */
            ToolServer(generation::PassState &state, data::Target &t)
                : m_state(state)
                , m_target(t)
            {
                // Empty.
            }
            /**
             * Dispatch one tool call.
             *
             * @param name Requested tool name.
             * @param arguments Tool arguments.
             * @return Tool response and error state.
             */
            data::ToolResult call_tool(llvm::StringRef name,
                                       const llvm::json::Object &arguments) override;
            /**
             * Build the target description supplied to the agent.
             *
             * @return Structured generation task.
             */
            llvm::json::Object get_task();
            /**
             * Build the enclosing compiler context supplied to the agent.
             *
             * @return Structured lexical and semantic context.
             */
            llvm::json::Object get_context();
            /** Return whether a candidate body has been accepted. @return Acceptance state. */
            bool accepted() const
            {
                return m_accepted;
            }
            /** Get accepted body statements. @return Accepted body statements. */
            const std::string &accepted_body() const
            {
                return m_accepted_body;
            }
            /** Get the last rejected body. @return Most recently rejected candidate body. */
            const std::string &last_attempt() const
            {
                return m_last_attempt;
            }
            /** Get rejection diagnostics. @return Diagnostics for the last rejected candidate. */
            const std::string &last_diagnostics() const
            {
                return m_last_diagnostics;
            }

        private:
            /** Look up a name. @param name Name to resolve. @return Lookup response. */
            data::ToolResult lookup(llvm::StringRef name);
            /**
             * List names from a namespace.
             *
             * @param namespaceName Namespace to inspect.
             * @param filter Case-insensitive name filter.
             * @param offset Pagination offset.
             * @return Paginated declaration response.
             */
            data::ToolResult list_namespace(llvm::StringRef namespaceName, llvm::StringRef filter,
                                            unsigned offset);
            /** Get attached documentation. @param name Declaration name. @return Tool response. */
            data::ToolResult get_comment(llvm::StringRef name);
            /** List included headers. @param all Whether to include transitive headers. @return
             * Tool response. */
            data::ToolResult included_headers(bool all);
            /** Compile a candidate. @param body Candidate statements. @return Tool response. */
            data::ToolResult try_compile(llvm::StringRef body);
            /** Submit a candidate. @param body Final candidate statements. @return Tool response.
             */
            data::ToolResult submit(llvm::StringRef body);
            /** Get the AST context. @return Active AST context. */
            ::clang::ASTContext &ctx() const;
            /** Get semantic analysis. @return Active semantic analyzer. */
            ::clang::Sema &sema() const;
            /** Get the source manager. @return Active source manager. */
            ::clang::SourceManager &sm() const;
            /** Find the lookup context. @return Class or namespace where lookup starts. */
            ::clang::DeclContext *lookup_start() const;
            /** Get the target location. @return Source location of the target body. */
            ::clang::SourceLocation target_loc() const;
            /** Check visibility. @param d Declaration to test. @return True when visible. */
            bool visible_here(const ::clang::NamedDecl *d) const;
            /**
             * Perform direct lookup in a declaration context.
             *
             * @param dc Context to search.
             * @param name Name to resolve.
             * @return Matching declarations.
             */
            std::vector<::clang::NamedDecl *> lookup_in(::clang::DeclContext *dc,
                                                        llvm::StringRef name);
            /**
             * Resolve a possibly qualified name.
             *
             * @param name Name to resolve.
             * @param error Destination for resolution diagnostics.
             * @return Matching declarations.
             */
            std::vector<::clang::NamedDecl *> resolve(llvm::StringRef name, std::string &error);
            /** Find visible locals. @return Parameters and captured locals visible at the target.
             */
            std::vector<const ::clang::VarDecl *> locals_in_scope() const;
            /** Describe a declaration. @param d Declaration to describe. @return Structured
             * details. */
            llvm::json::Object describe_decl(const ::clang::NamedDecl *d);
            /** Shared state for the active translation unit. */
            generation::PassState &m_state;
            /** data::Target currently being generated. */
            data::Target &m_target;
            /** Whether a candidate body has been accepted. */
            bool m_accepted = false;
            /** Number of rejected final-body submissions. */
            unsigned m_failed_submits = 0;
            /** Accepted body statements. */
            std::string m_accepted_body;
            /** Most recently rejected candidate body. */
            std::string m_last_attempt;
            /** Diagnostics associated with the most recent rejection. */
            std::string m_last_diagnostics;
        };

    }
}

#endif
