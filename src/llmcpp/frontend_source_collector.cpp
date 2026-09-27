/*
 * C++ file for collecting llm annotations and source inclusions.
 */

// Project header for annotation and inclusion collection.
#include "llmcpp/frontend_source_collector.h"

// Clang headers for source manager and preprocessor callback types.
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Token.h"

// Namespace for annotation and inclusion collection.
namespace llmcpp
{
    // Bind collection outputs to the active source manager.
    FrontendSourceCollector::FrontendSourceCollector(
        ::clang::SourceManager &sourceManager, std::vector<::clang::SourceLocation> &keywords,
        std::vector<data::DataIncludeDirective> &includes)
        : m_source_manager(sourceManager)
        , m_keywords(keywords)
        , m_includes(includes)
    {
        // Empty.
    }

    // Record an annotation macro expansion.
    void FrontendSourceCollector::MacroExpands(const ::clang::Token &name,
                                               const ::clang::MacroDefinition &,
                                               ::clang::SourceRange, const ::clang::MacroArgs *)
    {
        const ::clang::IdentifierInfo *identifier = name.getIdentifierInfo();
        if (identifier && identifier->getName() == "__llm__") {
            m_keywords.push_back(name.getLocation());
        }
    }

    // Record an include directive and resolved file path.
    void FrontendSourceCollector::InclusionDirective(
        ::clang::SourceLocation hashLoc, const ::clang::Token &, llvm::StringRef fileName,
        bool isAngled, ::clang::CharSourceRange, ::clang::OptionalFileEntryRef file,
        llvm::StringRef, llvm::StringRef, const ::clang::Module *, bool,
        ::clang::SrcMgr::CharacteristicKind)
    {
        data::DataIncludeDirective inc;
        inc.m_hash_loc = hashLoc;
        inc.m_spelled = isAngled ? "<" + fileName.str() + ">" : "\"" + fileName.str() + "\"";
        if (file) {
            inc.m_path = file->getName().str();
        }
        inc.m_from_main_file = m_source_manager.isInMainFile(hashLoc);
        m_includes.push_back(std::move(inc));
    }
}
