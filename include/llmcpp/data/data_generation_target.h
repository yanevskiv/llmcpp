/*
 * C++ header for one function or lambda targeted for LLM generation.
 */

#ifndef LLMCPP_DATA_GENERATION_TARGET_H
#define LLMCPP_DATA_GENERATION_TARGET_H

#include <string>
/** Namespace for required Clang forward declarations. */
namespace clang
{
    class CompoundStmt;
    class FunctionDecl;
    class LambdaExpr;
}
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Stores the declaration, prompt, cache identity, and generated code for one target. */
        struct DataGenerationTarget
        {
            /** Function declaration carrying the annotation. */
            ::clang::FunctionDecl *m_function = nullptr;
            /** Lambda expression carrying the annotation, when applicable. */
            ::clang::LambdaExpr *m_lambda = nullptr;
            /** Compound statement containing the generation prompt. */
            ::clang::CompoundStmt *m_body = nullptr;
            /** Source offset of the annotation spelling. */
            unsigned m_keyword_offset = 0;
            /** Source offset of the prompt body's opening brace. */
            unsigned m_l_brace = 0;
            /** Source offset of the prompt body's closing brace. */
            unsigned m_r_brace = 0;
            /** Stable display name of the target. */
            std::string m_name;
            /** Source signature supplied to the agent. */
            std::string m_signature;
            /** Human-readable annotation location. */
            std::string m_location;
            /** Normalized prompt text supplied to the agent. */
            std::string m_prompt_text;
            /** Stable cache key for the target. */
            std::string m_key;
            /** Whether generation has produced a body. */
            bool m_generated = false;
            /** Generated body statements. */
            std::string m_code;
            /** Model identifier associated with the generated body. */
            std::string m_model;
        };

    }
}

#endif
