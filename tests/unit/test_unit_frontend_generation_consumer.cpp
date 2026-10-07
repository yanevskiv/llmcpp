/*
 * C++ file for frontend generation consumer unit tests.
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

// Catch2 declarations for frontend checks.
#include <catch2/catch_test_macros.hpp>

// Project generation consumer, pass, and isolated compiler under test.
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/frontend_generation_consumer.h"
#include "llmcpp/generation_pass.h"

// Clang AST and compiler instance used for a completed translation unit.
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"

// Verify that the consumer forwards a completed AST to its generation pass.
TEST_CASE("generation consumer forwards a parsed translation unit",
          "[unit][frontend_generation_consumer]")
{
    std::string source = "int value = 7;";
    llmcpp::CompilerSandbox sandbox({"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"},
                                    "unit.cpp", LLMCPP_PATH);
    clang::CompilerInstance compiler;
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::data::DataGenerationResult result;
    llmcpp::GenerationPass pass(compiler, opts, {}, source, result);
    llmcpp::FrontendGenerationConsumer consumer(pass);
    llmcpp::data::DataCompilationResult compilation = sandbox.compile(
        source, 0, source.size(), false, "", [&](clang::ASTContext &context, clang::Sema &) {
            consumer.HandleTranslationUnit(context);
        });
    REQUIRE(compilation.m_ok);
    CHECK(pass.state().m_source == source);
    CHECK(result.m_status == llmcpp::data::DataGenerationStatus::Unchanged);
}
