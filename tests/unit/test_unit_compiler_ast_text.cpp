/*
 * C++ file for compiler AST text unit tests.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

// Catch2 declarations for AST-formatting checks.
#include <catch2/catch_test_macros.hpp>

// Project AST formatting and isolated compiler under test.
#include "llmcpp/compiler_ast_text.h"
#include "llmcpp/compiler_sandbox.h"

// Clang AST declarations inspected by the callback.
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"

// Verify that AST helpers render the function exposed to an agent.
TEST_CASE("AST text formats function declarations", "[unit][compiler_ast_text]")
{
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    bool inspected = false;
    std::string source = "int sum(int left, int right) { return left + right; }";
    llmcpp::data::DataCompilationResult result = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &context, clang::Sema &) {
            for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
                auto *function = llvm::dyn_cast<clang::FunctionDecl>(decl);
                if (!function || function->getName() != "sum") {
                    continue;
                }
                inspected = true;
                CHECK(llmcpp::kind_name(function) == "function");
                CHECK(llmcpp::print_type(function->getReturnType(), context) == "int");
                CHECK(llmcpp::function_signature(function, context).find("sum") !=
                      std::string::npos);
                CHECK(llmcpp::location_string(function->getLocation(),
                                              context.getSourceManager()) == "unit.cpp:1");
            }
        });
    REQUIRE(result.m_ok);
    CHECK(inspected);
}
