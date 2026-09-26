/*
 * C++ file for the llmc++ executable entry point.
 */

// Project header for driver application orchestration.
#include "llmcpp/driver/driver_app.h"

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
    llmcpp::driver::DriverApp app(argc, argv);
    return app.run();
}
