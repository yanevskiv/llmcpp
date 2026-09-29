/*
 * C++ header for formatting helpers for Clang AST entities.
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

#ifndef LLMCPP_COMPILER_AST_TEXT_H
#define LLMCPP_COMPILER_AST_TEXT_H

#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceLocation.h"

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /**
     * Format a Clang type for display to an agent.
     *
     * @param ty Type to format.
     * @param ctx AST context that owns the type.
     * @return Source-like spelling of the type.
     */
    std::string print_type(::clang::QualType ty, const ::clang::ASTContext &ctx);
    /**
     * Format a declaration without its body.
     *
     * @param d Declaration to format.
     * @param ctx AST context that owns the declaration.
     * @return Single-line declaration text.
     */
    std::string print_decl(const ::clang::Decl *d, const ::clang::ASTContext &ctx);
    /**
     * Return a human-readable declaration kind.
     *
     * @param d Declaration to classify.
     * @return Stable kind label.
     */
    std::string kind_name(const ::clang::NamedDecl *d);
    /**
     * Format a C++ access specifier.
     *
     * @param as Access specifier to format.
     * @return Static access label.
     */
    const char *access_name(::clang::AccessSpecifier as);
    /**
     * Check whether a declaration is accessible from a declaration context.
     *
     * @param d Declaration to test.
     * @param from Context from which the declaration would be referenced.
     * @return True when ordinary access control permits the reference.
     */
    bool accessible_from(const ::clang::NamedDecl *d, const ::clang::DeclContext *from);
    /**
     * Format a function signature.
     *
     * @param fd Function declaration to format.
     * @param ctx AST context that owns the declaration.
     * @return Qualified function signature.
     */
    std::string function_signature(const ::clang::FunctionDecl *fd, const ::clang::ASTContext &ctx);
    /**
     * Format a lambda call signature.
     *
     * @param le Lambda expression to format.
     * @param ctx AST context that owns the expression.
     * @return Lambda signature.
     */
    std::string lambda_signature(const ::clang::LambdaExpr *le, const ::clang::ASTContext &ctx);
    /**
     * Format a source location.
     *
     * @param loc Location to format.
     * @param sm Source manager used to resolve the location.
     * @return Filename, line, and column text.
     */
    std::string location_string(::clang::SourceLocation loc, const ::clang::SourceManager &sm);
}

#endif
