/*
 * C++ header for a source-text replacement.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++  is  free  software;  you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with llmc++; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LLMCPP_DATA_SOURCE_EDIT_H
#define LLMCPP_DATA_SOURCE_EDIT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Replaces a half-open source range and records its resulting offset. */
        struct DataSourceEdit
        {
            /** First replaced source offset. */
            unsigned m_begin;
            /** Offset just after the replaced source. */
            unsigned m_end;
            /** Replacement text. */
            std::string m_text;
            /** Offset of this edit after prior edits are applied. */
            unsigned m_new_begin = 0;
        };

    }
}

#endif
