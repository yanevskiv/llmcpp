/*
 * C++ file for creating shadow-compilation inspection consumers.
 */

// Project headers for shadow AST inspection and actions.
#include "llmcpp/compiler_inspection_action.h"
#include "llmcpp/compiler_inspection_consumer.h"

// Namespace for shadow AST inspection actions.
namespace llmcpp
{
    // Store the inspection callback for consumer construction.
    CompilerInspectionAction::CompilerInspectionAction(InspectFn inspect)
        : m_inspect(inspect)
    {
        // Empty.
    }

    // Create the consumer that receives the shadow AST.
    std::unique_ptr<::clang::ASTConsumer>
    CompilerInspectionAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
    {
        return std::make_unique<CompilerInspectionConsumer>(ci, m_inspect);
    }
}
