/*
 * C++ header for one function or lambda targeted for LLM generation.
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

#ifndef LLMCPP_DATA_GENERATION_TARGET_H
#define LLMCPP_DATA_GENERATION_TARGET_H

#include "llmcpp/data/data_generation_options.h"
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
            /** Source offset immediately after the complete modifier. */
            unsigned m_keyword_end = 0;
            /** Effective generation policy for this target. */
            DataGenerationOptions m_options;
            /** Explicit project salt for this target's cache identity. */
            std::string m_cache_salt;
            /** Explicit cache identity or unambiguous hexadecimal prefix supplied by the user. */
            std::string m_cache_key;
            /** Whether the modifier overrides the external agent command. */
            bool m_agent_override = false;
            /** Digest of compilation arguments and visible source contents. */
            std::string m_context_digest;
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
            /** Original generation timestamp retained across cache hits. */
            std::string m_date;
            /** Agent identity recorded when the implementation was generated. */
            std::string m_agent_identity;
        };

    }
}

#endif
