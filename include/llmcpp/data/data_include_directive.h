/*
 * C++ header for metadata captured for an include directive.
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

#ifndef LLMCPP_DATA_INCLUDE_DIRECTIVE_H
#define LLMCPP_DATA_INCLUDE_DIRECTIVE_H

#include "clang/Basic/SourceLocation.h"

#include <string>
/** Namespace for llmcpp declarations. */
namespace llmcpp
{
    /** Namespace for llmcpp data declarations. */
    namespace data
    {
        /** Describes one include directive observed while parsing a translation unit. */
        struct DataIncludeDirective
        {
            /** Source location of the include directive's hash token. */
            ::clang::SourceLocation m_hash_loc;
            /** Include filename as written in source. */
            std::string m_spelled;
            /** Resolved path of the included file. */
            std::string m_path;
            /** Whether the directive belongs to the main source file. */
            bool m_from_main_file = false;
        };

    }
}

#endif
