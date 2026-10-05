/*
 * C++ header for lexical token ranges used while finding prompt bodies.
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

#ifndef LLMCPP_DATA_PROMPT_TOKEN_H
#define LLMCPP_DATA_PROMPT_TOKEN_H
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Stores a token range relevant to prompt-body discovery. */
        struct DataPromptToken
        {
            /** First source offset covered by the token. */
            unsigned m_begin;
            /** Offset just after the token. */
            unsigned m_end;
            /** Whether the token occurs in a preprocessor directive. */
            bool m_directive;
        };

    }
}

#endif
