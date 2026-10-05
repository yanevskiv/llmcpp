/*
 * C++ file for the llmc++ executable entry point.
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

// Project header for driver application orchestration.
#include "llmcpp/driver_app.h"

// LLVM headers for process initialization and target registration.
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

// Run the llmc++ executable.
int main(int argc, char **argv)
{
    llvm::InitLLVM llvm(argc, argv);
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    llmcpp::DriverApp app(argc, argv);
    return app.run();
}
