/*
 * C++ header for diagnostics returned by a shadow compilation.
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

#ifndef LLMCPP_DATA_COMPILATION_RESULT_H
#define LLMCPP_DATA_COMPILATION_RESULT_H

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Summarizes diagnostics from an isolated shadow compilation. */
        struct DataCompilationResult
        {
            /** Whether shadow compilation completed without errors. */
            bool m_ok = false;
            /** Number of filtered error diagnostics. */
            unsigned m_errors = 0;
            /** Number of filtered warning diagnostics. */
            unsigned m_warnings = 0;
            /** Formatted diagnostics from shadow compilation. */
            std::string m_text;
        };

    }
}

#endif
