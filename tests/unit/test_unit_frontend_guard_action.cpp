/*
 * C++ file for frontend guard action unit tests.
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

// Catch2 declarations for guard checks.
#include <catch2/catch_test_macros.hpp>

// Project guard action under test.
#include "llmcpp/frontend_guard_action.h"

// Clang frontend configuration and syntax-only action wrapped by the guard.
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/PreprocessorOptions.h"

// LLVM memory buffer for in-memory source.
#include "llvm/Support/MemoryBuffer.h"

// Standard shared ownership for the frontend invocation.
#include <memory>

// Verify that the guard diagnoses an annotation before ordinary compilation.
TEST_CASE("guard action rejects an annotated source", "[unit][frontend_guard_action]")
{
    const char *args[] = {"-x", "c++", "-std=c++20", "-fsyntax-only", "unit.cpp"};
    auto invocation = std::make_shared<clang::CompilerInvocation>();
    clang::IgnoringDiagConsumer ignored;
    clang::DiagnosticsEngine diagnostics(new clang::DiagnosticIDs(), new clang::DiagnosticOptions(),
                                         &ignored, false);
    REQUIRE(clang::CompilerInvocation::CreateFromArgs(*invocation, args, diagnostics, LLMCPP_PATH));
    std::string source = "#define __llm__\n__llm__ int value = 7;";
    invocation->getPreprocessorOpts().addRemappedFile(
        "unit.cpp", llvm::MemoryBuffer::getMemBufferCopy(source, "unit.cpp").release());
    clang::CompilerInstance compiler;
    compiler.setInvocation(std::move(invocation));
    compiler.createDiagnostics();
    llmcpp::FrontendGuardAction action(std::make_unique<clang::SyntaxOnlyAction>());
    CHECK_FALSE(compiler.ExecuteAction(action));
    CHECK(compiler.getDiagnostics().hasErrorOccurred());
}
