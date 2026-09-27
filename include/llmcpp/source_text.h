/*
 * C++ header for text helpers for prompts, hashing, and source rewriting.
 */

#ifndef LLMCPP_SOURCE_TEXT_H
#define LLMCPP_SOURCE_TEXT_H

#include "llmcpp/data/data_source_edit.h"

#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /**
     * Apply non-overlapping source edits in position order.
     *
     * @param source Original source text.
     * @param edits Mutable edits whose resulting offsets are recorded.
     * @return Rewritten source text.
     */
    std::string apply_edits(llvm::StringRef source, std::vector<data::DataSourceEdit> &edits);
    /**
     * Remove trailing whitespace, outer blank lines, and common indentation.
     *
     * @param text Text to normalize.
     * @return Dedented text.
     */
    std::string dedent(llvm::StringRef text);
    /**
     * Dedent code and prefix every non-empty line.
     *
     * @param code Code to reindent.
     * @param indent Prefix for non-empty lines.
     * @return Reindented text with newline-terminated lines.
     */
    std::string reindent(llvm::StringRef code, llvm::StringRef indent);
    /**
     * Get leading whitespace for the line containing an offset.
     *
     * @param source Source text to inspect.
     * @param offset Byte offset within the source.
     * @return View of the line's leading whitespace.
     */
    llvm::StringRef line_indent(llvm::StringRef source, unsigned offset);
    /**
     * Check whether only whitespace precedes an offset on its line.
     *
     * @param source Source text to inspect.
     * @param offset Byte offset within the source.
     * @return True when the offset is the first non-whitespace position.
     */
    bool first_on_line(llvm::StringRef source, unsigned offset);
    /**
     * Collapse whitespace runs and trim the result.
     *
     * @param text Text to normalize.
     * @return Single-line normalized text.
     */
    std::string collapse_whitespace(llvm::StringRef text);
    /**
     * Compute a lowercase SHA-256 digest.
     *
     * @param data Bytes to hash.
     * @return Hexadecimal digest.
     */
    std::string sha256_hex(llvm::StringRef data);
}

#endif
