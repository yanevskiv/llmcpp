/*
 * C++ file for frontend generation action unit tests.
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

// Project generation action and pass under test.
#include "llmcpp/frontend_generation_action.h"
#include "llmcpp/generation_pass.h"

// Clang frontend configuration for a remapped translation unit.
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Lex/PreprocessorOptions.h"

// LLVM memory buffer for in-memory source.
#include "llvm/Support/MemoryBuffer.h"

// Standard shared ownership for the frontend invocation.
#include <memory>

// Verify that the action installs its consumer and processes source without targets.
TEST_CASE("generation action parses a translation unit", "[unit][frontend_generation_action]")
{
    const char *args[] = {"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"};
    auto invocation = std::make_shared<clang::CompilerInvocation>();
    clang::IgnoringDiagConsumer ignored;
    clang::DiagnosticsEngine diagnostics(new clang::DiagnosticIDs(), new clang::DiagnosticOptions(),
                                         &ignored, false);
    REQUIRE(clang::CompilerInvocation::CreateFromArgs(*invocation, args, diagnostics, LLMCPP_PATH));
    std::string source = "int value = 7;";
    invocation->getPreprocessorOpts().addRemappedFile(
        "unit.cpp", llvm::MemoryBuffer::getMemBufferCopy(source, "unit.cpp").release());
    clang::CompilerInstance compiler;
    compiler.setInvocation(std::move(invocation));
    compiler.createDiagnostics();
    llmcpp::data::DataGenerationOptions opts;
    llmcpp::data::DataGenerationResult result;
    llmcpp::GenerationPass pass(compiler, opts, {}, source, result);
    pass.state().m_main_file = "unit.cpp";
    llmcpp::FrontendGenerationAction action(pass);
    REQUIRE(compiler.ExecuteAction(action));
    CHECK(pass.state().m_source == source);
    CHECK(result.m_status == llmcpp::data::DataGenerationStatus::Unchanged);
}
