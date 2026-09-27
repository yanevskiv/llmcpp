/*
 * C++ header for metadata captured for an include directive.
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
