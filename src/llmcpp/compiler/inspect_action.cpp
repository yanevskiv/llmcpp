/*
 * C++ file for creating shadow-compilation inspection consumers.
 */

// Project headers for shadow AST inspection and actions.
#include "llmcpp/compiler/inspect_action.h"
#include "llmcpp/compiler/inspect_consumer.h"

// Namespace for shadow AST inspection actions.
namespace llmcpp
{
    // Namespace for llmcpp compiler implementation.
    namespace compiler
    {
        // Store the inspection callback for consumer construction.
        InspectAction::InspectAction(InspectFn inspect)
            : m_inspect(inspect)
        {
            // Empty.
        }

        // Create the consumer that receives the shadow AST.
        std::unique_ptr<::clang::ASTConsumer>
        InspectAction::CreateASTConsumer(::clang::CompilerInstance &ci, llvm::StringRef)
        {
            return std::make_unique<InspectConsumer>(ci, m_inspect);
        }
    }
}
