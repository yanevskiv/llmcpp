/*
 * C++ header for a compiler-context tool response.
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

#ifndef LLMCPP_DATA_TOOL_RESULT_H
#define LLMCPP_DATA_TOOL_RESULT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Contains the text and error state returned by a compiler-context tool. */
        struct DataToolResult
        {
            /** Textual response returned to the agent. */
            std::string m_text;
            /** Whether the response represents a tool error. */
            bool m_is_error = false;
        };

    }
}

#endif
