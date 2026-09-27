/*
 * C++ file for prompt normalization, hashing, and source-text helpers.
 */

// Project header for prompt and source text helpers.
#include "llmcpp/source_text.h"

// LLVM headers for hashing and text algorithms.
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"

// Standard library header for text algorithms.
#include <algorithm>

// Type alias for lightweight LLVM string views.
using llvm::StringRef;

// Namespace for llmcpp text transformations.
namespace llmcpp
{

    // Apply ordered source replacements and record resulting offsets.
    std::string apply_edits(StringRef source, std::vector<data::DataSourceEdit> &edits)
    {
        llvm::sort(edits, [](const data::DataSourceEdit &a, const data::DataSourceEdit &b) {
            return a.m_begin < b.m_begin;
        });
        std::string out;
        out.reserve(source.size());
        unsigned pos = 0;
        for (data::DataSourceEdit &e : edits) {
            out.append(source.data() + pos, e.m_begin - pos);
            e.m_new_begin = out.size();
            out += e.m_text;
            pos = e.m_end;
        }
        out.append(source.data() + pos, source.size() - pos);
        return out;
    }

    // Split text into lines while trimming trailing whitespace.
    static std::vector<std::string> split_lines(StringRef text)
    {
        llvm::SmallVector<StringRef, 16> parts;
        text.split(parts, '\n');
        std::vector<std::string> lines;
        for (StringRef p : parts) {
            lines.push_back(p.rtrim().str());
        }
        return lines;
    }

    // Test whether a line contains only whitespace.
    static bool is_blank(StringRef line)
    {
        return line.trim().empty();
    }

    // Remove outer whitespace and common indentation from lines.
    static std::string join_dedented(std::vector<std::string> lines)
    {
        while (!lines.empty() && is_blank(lines.back())) {
            lines.pop_back();
        }
        auto first = llvm::find_if_not(lines, [](const std::string &l) {
            return is_blank(l);
        });
        lines.erase(lines.begin(), first);

        size_t common = std::string::npos;
        for (const std::string &l : lines) {
            if (!is_blank(l)) {
                common = std::min(common, l.find_first_not_of(" \t"));
            }
        }

        std::string out;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i) {
                out += '\n';
            }
            if (!is_blank(lines[i])) {
                out += lines[i].substr(common);
            }
        }
        return out;
    }

    // Remove outer blank lines and common indentation.
    std::string dedent(StringRef text)
    {
        return join_dedented(split_lines(text));
    }

    // Apply a new indentation prefix to code.
    std::string reindent(StringRef code, StringRef indent)
    {
        std::string d = dedent(code);
        if (d.empty()) {
            return "";
        }
        llvm::SmallVector<StringRef, 32> lines;
        StringRef(d).split(lines, '\n');
        std::string out;
        for (StringRef l : lines) {
            if (!l.empty()) {
                out += indent;
            }
            out += l;
            out += '\n';
        }
        return out;
    }

    // Return text preceding an offset on the same line.
    static StringRef line_before(StringRef source, unsigned offset)
    {
        StringRef before = source.substr(0, offset);
        size_t nl = before.rfind('\n');
        return before.substr(nl == StringRef::npos ? 0 : nl + 1);
    }

    // Return the indentation of the line containing an offset.
    StringRef line_indent(StringRef source, unsigned offset)
    {
        StringRef line = line_before(source, offset);
        size_t n = line.find_first_not_of(" \t");
        return line.substr(0, n == StringRef::npos ? line.size() : n);
    }

    // Test whether an offset is the first non-whitespace position on a line.
    bool first_on_line(StringRef source, unsigned offset)
    {
        return is_blank(line_before(source, offset));
    }

    // Collapse whitespace runs into single spaces.
    std::string collapse_whitespace(StringRef text)
    {
        std::string out;
        bool space = false;
        for (char c : text) {
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
                space = !out.empty();
                continue;
            }
            if (space) {
                out += ' ';
            }
            space = false;
            out += c;
        }
        return out;
    }

    // Compute a lowercase SHA-256 digest.
    std::string sha256_hex(StringRef data)
    {
        llvm::SHA256 h;
        h.update(data);
        std::array<uint8_t, 32> hash = h.final();
        return llvm::toHex(hash, true);
    }

}
