/*
 * C++ file for prompt discovery, generation, caching, and source rewriting.
 */

// Project headers for pass state, tools, transport, and text handling.
#include "llmcpp/generation_pass.h"
#include "llmcpp/agent_prompt.h"
#include "llmcpp/agent_session.h"
#include "llmcpp/agent_tool_server.h"
#include "llmcpp/compiler_ast_text.h"
#include "llmcpp/compiler_sandbox.h"
#include "llmcpp/data/data_prompt_token.h"
#include "llmcpp/frontend_candidate_collector.h"
#include "llmcpp/frontend_generation_action.h"
#include "llmcpp/frontend_source_collector.h"
#include "llmcpp/generation_context.h"
#include "llmcpp/source_text.h"

// Clang headers for AST traversal, preprocessing, and frontend execution.
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

// LLVM headers for containers, files, formatting, and output.
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

// Standard library headers for lexing, time, and collections.
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <memory>
#include <optional>
#include <set>

// Namespace import for Clang AST and frontend types.
using namespace clang;
// Function alias for LLVM format helpers.
using llvm::formatv;
// Type alias for lightweight LLVM string views.
using llvm::StringRef;
// Namespace alias for LLVM JSON types.
namespace json = llvm::json;

// Namespace for the public llmcpp compilation pass.
namespace llmcpp
{
    // Namespace for pass implementation details private to this translation unit.
    namespace
    {

        // Constants for lexical markers used while discovering prompt bodies.
        const char Keyword[] = "__llm__";
        const char CacheVersion[] = "llmcpp-cache-3";

        // Recognize the first character of an identifier.
        static bool is_identifier_start(char c)
        {
            return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
        }

        // Recognize a subsequent character of an identifier.
        static bool is_identifier_body(char c)
        {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }

        // Find the end of a raw string beginning at the requested offset.
        static std::optional<unsigned> raw_string_end(StringRef source, unsigned begin)
        {
            static constexpr StringRef prefixes[] = {"R\"", "u8R\"", "uR\"", "UR\"", "LR\""};
            StringRef prefix;
            for (StringRef candidate : prefixes) {
                if (source.substr(begin).starts_with(candidate)) {
                    prefix = candidate;
                    break;
                }
            }
            if (prefix.empty()) {
                return std::nullopt;
            }

            unsigned delimiterBegin = begin + prefix.size();
            size_t openParen = source.find('(', delimiterBegin);
            if (openParen == StringRef::npos || openParen - delimiterBegin > 16) {
                return std::nullopt;
            }
            StringRef delimiter = source.slice(delimiterBegin, openParen);
            if (delimiter.find_first_of(" \\\t\r\n)") != StringRef::npos) {
                return std::nullopt;
            }
            std::string terminator = ")" + delimiter.str() + "\"";
            size_t close = source.find(terminator, openParen + 1);
            if (close == StringRef::npos) {
                return source.size();
            }
            return static_cast<unsigned>(close + terminator.size());
        }

        // Tokenize source while excluding braces embedded in comments and literals.
        static std::vector<data::DataPromptToken> source_tokens(StringRef source)
        {
            std::vector<data::DataPromptToken> tokens;
            bool atLineStart = true, directive = false;
            for (unsigned i = 0; i < source.size();) {
                char c = source[i];
                if (c == '\n' || c == '\r') {
                    ++i;
                    atLineStart = true;
                    directive = false;
                    continue;
                }
                if (c == ' ' || c == '\t' || c == '\f' || c == '\v') {
                    ++i;
                    continue;
                }
                if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
                    i += 2;
                    while (i < source.size() && source[i] != '\n' && source[i] != '\r') {
                        ++i;
                    }
                    continue;
                }
                if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
                    i += 2;
                    while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/')) {
                        if (source[i] == '\n' || source[i] == '\r') {
                            atLineStart = true;
                            directive = false;
                        }
                        ++i;
                    }
                    i = std::min<unsigned>(i + 2, source.size());
                    continue;
                }
                if (atLineStart && c == '#') {
                    directive = true;
                }
                atLineStart = false;

                if (std::optional<unsigned> end = raw_string_end(source, i)) {
                    tokens.push_back({i, *end, directive});
                    i = *end;
                    continue;
                }

                if (c == '"' || c == '\'') {
                    char quote = c;
                    unsigned begin = i++;
                    while (i < source.size() && source[i] != '\n' && source[i] != '\r') {
                        if (source[i] == '\\' && i + 1 < source.size()) {
                            i += 2;
                            continue;
                        }
                        if (source[i++] == quote) {
                            break;
                        }
                    }
                    tokens.push_back({begin, i, directive});
                    continue;
                }
                if (is_identifier_start(c)) {
                    unsigned begin = i++;
                    while (i < source.size() && is_identifier_body(source[i])) {
                        ++i;
                    }
                    tokens.push_back({begin, i, directive});
                    continue;
                }
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    unsigned begin = i++;
                    while (i < source.size() &&
                           std::isdigit(static_cast<unsigned char>(source[i]))) {
                        ++i;
                    }
                    tokens.push_back({begin, i, directive});
                    continue;
                }
                tokens.push_back({i, i + 1, directive});
                ++i;
            }
            return tokens;
        }

        // Compare a source token with expected spelling.
        static bool token_is(StringRef source, const data::DataPromptToken &t, StringRef text)
        {
            return source.slice(t.m_begin, t.m_end) == text;
        }

        // Locate the closing parenthesis of a function-like modifier.
        static size_t modifier_end(StringRef source, llvm::ArrayRef<data::DataPromptToken> tokens,
                                   size_t keyword)
        {
            if (keyword + 1 >= tokens.size() || !token_is(source, tokens[keyword + 1], "(")) {
                return keyword;
            }
            unsigned depth = 0;
            for (size_t i = keyword + 1; i < tokens.size(); ++i) {
                if (token_is(source, tokens[i], "(")) {
                    ++depth;
                } else if (token_is(source, tokens[i], ")") && --depth == 0) {
                    return i;
                }
            }
            return keyword;
        }

        // Resolve a small declarative option list without evaluating C++ expressions.
        static bool parse_target_options(StringRef source, data::DataGenerationTarget &target,
                                         std::string &error)
        {
            auto tokens = source_tokens(source.drop_front(target.m_keyword_offset));
            StringRef text = source.drop_front(target.m_keyword_offset);
            size_t end = modifier_end(text, tokens, 0);
            if (end == 0) {
                target.m_keyword_end = target.m_keyword_offset + tokens[0].m_end;
                if (tokens.size() > 1 && token_is(text, tokens[1], "(")) {
                    error = "unterminated __llm__ option list";
                    return false;
                }
                return true;
            }
            target.m_keyword_end = target.m_keyword_offset + tokens[end].m_end;
            std::set<std::string> seen;
            std::vector<std::string> appendFiles;
            for (size_t i = 2; i < end;) {
                std::string name = text.slice(tokens[i].m_begin, tokens[i].m_end).str();
                if (!seen.insert(name).second && name != "append_system_prompt") {
                    error = "duplicate __llm__ option '" + name + "'";
                    return false;
                }
                ++i;
                if (name == "no_cache") {
                    target.m_options.m_use_cache = false;
                } else if (name == "offline") {
                    target.m_options.m_offline = true;
                } else if (name == "force_regenerate") {
                    target.m_options.m_force_regenerate = true;
                } else if (name == "dump") {
                    target.m_options.m_dump = true;
                } else if (name == "dump_context") {
                    target.m_options.m_dump_context = true;
                } else if (name == "verbose") {
                    target.m_options.m_verbose = true;
                } else {
                    if (i + 2 >= end || !token_is(text, tokens[i], "(") ||
                        !token_is(text, tokens[i + 2], ")")) {
                        error = "expected one literal argument for __llm__ option '" + name + "'";
                        return false;
                    }
                    StringRef literal = text.slice(tokens[i + 1].m_begin, tokens[i + 1].m_end);
                    if (name == "model" || name == "cache_salt" || name == "key" ||
                        name == "backend" || name == "agent" || name == "cache_dir" ||
                        name == "system_prompt" || name == "append_system_prompt" ||
                        name == "agent_config" || name == "transcript") {
                        auto value = json::parse(literal);
                        if (!value) {
                            llvm::consumeError(value.takeError());
                            error = "expected a quoted string for __llm__ option '" + name + "'";
                            return false;
                        }
                        auto string = value->getAsString();
                        if (!string || string->empty()) {
                            error = "expected a nonempty string for __llm__ option '" + name + "'";
                            return false;
                        }
                        if (name == "model") {
                            target.m_options.m_model = string->str();
                        } else if (name == "append_system_prompt") {
                            appendFiles.push_back(string->str());
                        } else if (name == "transcript") {
                            target.m_options.m_transcript_file = string->str();
                        } else if (name == "agent_config") {
                            auto buffer = llvm::MemoryBuffer::getFile(*string);
                            if (!buffer) {
                                error = "cannot read agent configuration '" + string->str() +
                                        "': " + buffer.getError().message();
                                return false;
                            }
                            auto config = json::parse((*buffer)->getBuffer());
                            if (!config) {
                                llvm::consumeError(config.takeError());
                                error = "agent configuration must be a JSON object";
                                return false;
                            }
                            if (!config->getAsObject()) {
                                error = "agent configuration must be a JSON object";
                                return false;
                            }
                            target.m_options.m_agent_config_file = string->str();
                            target.m_options.m_agent_config = (*buffer)->getBuffer().str();
                        } else if (name == "system_prompt") {
                            auto buffer = llvm::MemoryBuffer::getFile(*string);
                            if (!buffer) {
                                error = "cannot read system prompt '" + string->str() +
                                        "': " + buffer.getError().message();
                                return false;
                            }
                            if (!json::isUTF8((*buffer)->getBuffer())) {
                                error = "system prompt '" + string->str() + "' is not UTF-8";
                                return false;
                            }
                            target.m_options.m_system_prompt = (*buffer)->getBuffer().str();
                            target.m_options.m_system_prompt_file = string->str();
                            target.m_options.m_append_system_prompt_files.clear();
                        } else if (name == "cache_dir") {
                            target.m_options.m_cache_dir = string->str();
                        } else if (name == "agent") {
                            if (string->trim().empty()) {
                                error = "expected a nonblank command for __llm__ option 'agent'";
                                return false;
                            }
                            target.m_options.m_agent_command = string->str();
                            target.m_agent_override = true;
                        } else if (name == "backend") {
                            if (*string != "anthropic" && *string != "openai" &&
                                *string != "codex" && *string != "claude") {
                                error = "unknown LLM backend '" + string->str() + "'";
                                return false;
                            }
                            target.m_options.m_backend = string->str();
                        } else if (name == "key") {
                            if (string->size() < 7 || string->size() > 64 ||
                                string->find_first_not_of("0123456789abcdefABCDEF") !=
                                    StringRef::npos) {
                                error = "expected 7 through 64 hexadecimal characters for __llm__ "
                                        "option 'key'";
                                return false;
                            }
                            target.m_cache_key = string->lower();
                        } else {
                            target.m_cache_salt = string->str();
                            target.m_options.m_use_cache = true;
                        }
                    } else if (name == "cache_lifetime" || name == "max_attempts" ||
                               name == "max_tool_calls" || name == "timeout") {
                        unsigned value = 0;
                        if (literal.getAsInteger(10, value) ||
                            (!value && name != "cache_lifetime")) {
                            error =
                                "expected a " +
                                std::string(name == "cache_lifetime" ? "nonnegative" : "positive") +
                                " integer for __llm__ option '" + name + "'";
                            return false;
                        }
                        if (name == "cache_lifetime") {
                            target.m_options.m_cache_lifetime = value;
                        } else if (name == "max_attempts") {
                            target.m_options.m_max_attempts = value;
                        } else if (name == "max_tool_calls") {
                            target.m_options.m_max_tool_calls = value;
                        } else {
                            target.m_options.m_timeout_seconds = value;
                        }
                    } else {
                        error = "unknown __llm__ option '" + name + "'";
                        return false;
                    }
                    i += 3;
                }
                if (i < end && (!token_is(text, tokens[i++], ",") || i == end)) {
                    error = "expected another __llm__ option after a comma";
                    return false;
                }
            }
            for (const std::string &file : appendFiles) {
                auto buffer = llvm::MemoryBuffer::getFile(file);
                if (!buffer) {
                    error =
                        "cannot read system prompt '" + file + "': " + buffer.getError().message();
                    return false;
                }
                if (!json::isUTF8((*buffer)->getBuffer())) {
                    error = "system prompt '" + file + "' is not UTF-8";
                    return false;
                }
                if (!target.m_options.m_system_prompt.empty()) {
                    target.m_options.m_system_prompt += "\n\n";
                }
                target.m_options.m_system_prompt += (*buffer)->getBuffer().str();
                target.m_options.m_append_system_prompt_files.push_back(file);
            }
            if (seen.count("cache_salt") && seen.count("no_cache")) {
                error = "cache_salt and no_cache cannot be combined";
                return false;
            }
            if (seen.count("offline") && seen.count("no_cache")) {
                error = "offline and no_cache cannot be combined";
                return false;
            }
            if (seen.count("offline")) {
                target.m_options.m_use_cache = true;
            }
            if (seen.count("key") && seen.count("no_cache")) {
                error = "key and no_cache cannot be combined";
                return false;
            }
            if (seen.count("key")) {
                target.m_options.m_use_cache = true;
            }
            return true;
        }

        // Find the closing brace paired with a prompt body's opening brace.
        static std::optional<unsigned>
        matching_prompt_brace(StringRef source, llvm::ArrayRef<data::DataPromptToken> tokens,
                              size_t lBraceToken)
        {
            unsigned depth = 0;
            for (size_t i = lBraceToken; i < tokens.size(); ++i) {
                if (token_is(source, tokens[i], "{")) {
                    ++depth;
                } else if (token_is(source, tokens[i], "}") && depth && --depth == 0) {
                    return tokens[i].m_begin;
                }
            }
            return std::nullopt;
        }

        // Produce a coordinate-preserving source copy with prompt prose erased.
        static std::string make_parseable_source_impl(StringRef source)
        {
            std::vector<data::DataPromptToken> tokens = source_tokens(source);
            std::string result = source.str();
            for (size_t i = 0; i < tokens.size();) {
                if (tokens[i].m_directive || !token_is(source, tokens[i], Keyword)) {
                    ++i;
                    continue;
                }

                size_t modifierEnd = modifier_end(source, tokens, i);
                for (unsigned p = tokens[i].m_end; p < tokens[modifierEnd].m_end; ++p) {
                    if (result[p] != '\n' && result[p] != '\r') {
                        result[p] = ' ';
                    }
                }
                unsigned parens = 0, squares = 0;
                bool sawParameterList = false, inCtorInitializers = false;
                size_t bodyOpen = tokens.size();
                unsigned bodyCloseOffset = 0;
                for (size_t j = modifierEnd + 1; j < tokens.size(); ++j) {
                    StringRef text = source.slice(tokens[j].m_begin, tokens[j].m_end);
                    if (text == "(") {
                        ++parens;
                    } else if (text == ")" && parens) {
                        if (--parens == 0) {
                            sawParameterList = true;
                        }
                    } else if (text == "[") {
                        ++squares;
                    } else if (text == "]" && squares) {
                        --squares;
                    } else if (parens == 0 && squares == 0 && text == ":" && sawParameterList) {
                        inCtorInitializers = true;
                    } else if (parens == 0 && squares == 0 && (text == ";" || text == "=")) {
                        break;
                    } else if (parens == 0 && squares == 0 && text == "{") {
                        if (inCtorInitializers) {
                            unsigned depth = 1;
                            size_t k = j + 1;
                            for (; k < tokens.size() && depth; ++k) {
                                if (token_is(source, tokens[k], "{")) {
                                    ++depth;
                                } else if (token_is(source, tokens[k], "}")) {
                                    --depth;
                                }
                            }
                            if (!depth) {
                                size_t close = k - 1;
                                StringRef next = k < tokens.size() ? source.slice(tokens[k].m_begin,
                                                                                  tokens[k].m_end)
                                                                   : StringRef();
                                if (next == "," || next == "{") {
                                    j = close;
                                    continue;
                                }
                            }
                        }

                        bodyOpen = j;
                        std::optional<unsigned> close = matching_prompt_brace(source, tokens, j);
                        if (!close) {
                            bodyOpen = tokens.size();
                            break;
                        }
                        bodyCloseOffset = *close;
                        break;
                    }
                }

                if (bodyOpen == tokens.size()) {
                    ++i;
                    continue;
                }
                for (unsigned p = tokens[bodyOpen].m_end; p < bodyCloseOffset; ++p) {
                    if (result[p] != '\n' && result[p] != '\r') {
                        result[p] = ' ';
                    }
                }
                ++i;
                while (i < tokens.size() && tokens[i].m_begin <= bodyCloseOffset) {
                    ++i;
                }
            }
            return result;
        }

        // Format the current UTC time for cache metadata.
        std::string current_time()
        {
            std::time_t now = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
            return buf;
        }

    }

    // Create a parseable source copy with generation prompts erased.
    std::string make_parseable_source(StringRef source)
    {
        return make_parseable_source_impl(source);
    }

    // Initialize a pass over a compiler invocation.
    GenerationPass::GenerationPass(CompilerInstance &ci, const data::DataGenerationOptions &opts,
                                   std::vector<std::string> cc1Args, std::string originalSource,
                                   data::DataGenerationResult &result)
        : m_state(ci, opts)
        , m_ci(ci)
        , m_opts(opts)
        , m_result(result)
        , m_cc1_args(std::move(cc1Args))
        , m_original_source(std::move(originalSource))
    {
        // Empty.
    }

    // Return annotation locations collected during preprocessing.
    std::vector<SourceLocation> &GenerationPass::keyword_locations()
    {
        return m_keywords;
    }

    // Return shared state for the active compilation pass.
    GenerationContext &GenerationPass::state()
    {
        return m_state;
    }

    // Emit a pass diagnostic using a compile-time format string.
    template <unsigned N>
    DiagnosticBuilder GenerationPass::report(SourceLocation location,
                                             DiagnosticsEngine::Level level,
                                             const char (&format)[N])
    {
        DiagnosticsEngine &diagnostics = m_ci.getDiagnostics();
        return diagnostics.Report(location, diagnostics.getCustomDiagID(level, format));
    }

    // Run generation and rewriting after parsing succeeds.
    void GenerationPass::run(ASTContext &ctx)
    {
        DiagnosticsEngine &diags = ctx.getDiagnostics();
        if (diags.hasErrorOccurred()) {
            m_result.m_status = data::DataGenerationStatus::Failed;
            return;
        }
        m_state.m_source = m_original_source;
        if (m_keywords.empty()) {
            return;
        }

        collect_targets(ctx);
        if (diags.hasErrorOccurred()) {
            m_result.m_status = data::DataGenerationStatus::Failed;
            return;
        }
        if (m_state.m_targets.empty()) {
            return;
        }

        m_state.m_shadow =
            std::make_unique<CompilerSandbox>(m_cc1_args, m_state.m_main_file, m_opts.m_executable);

        bool dumpContext = false;
        for (const data::DataGenerationTarget &t : m_state.m_targets) {
            dumpContext |= t.m_options.m_dump_context;
        }
        if (dumpContext) {
            for (data::DataGenerationTarget &t : m_state.m_targets) {
                if (!t.m_options.m_dump_context) {
                    continue;
                }
                AgentToolServer tools(m_state, t);
                json::Object o{{"task", tools.get_task()}, {"context", tools.get_context()}};
                llvm::outs() << formatv("{0:2}", json::Value(std::move(o))) << "\n";
            }
            m_result.m_status = data::DataGenerationStatus::DumpedContext;
            return;
        }

        bool ok = true;
        for (data::DataGenerationTarget &t : m_state.m_targets) {
            ok &= generate(t);
        }
        if (!ok) {
            m_result.m_status = data::DataGenerationStatus::Failed;
            return;
        }

        std::string output = rewrite();
        if (m_opts.m_emit_source) {
            data::DataCompilationResult r = m_state.m_shadow->compile(
                output, 0, 0, false, llvm::sys::path::filename(m_state.m_main_file));
            if (!r.m_ok) {
                report(SourceLocation(), DiagnosticsEngine::Error,
                       "the rewritten '%0' does not compile:\n%1")
                    << llvm::sys::path::filename(m_state.m_main_file) << r.m_text;
                m_result.m_status = data::DataGenerationStatus::Failed;
                return;
            }
        }
        m_result.m_status = data::DataGenerationStatus::Rewritten;
        m_result.m_output = std::move(output);
    }

    // Find the next raw token at or after a source offset.
    static unsigned next_token_offset(const SourceManager &sm, const LangOptions &lo, StringRef src,
                                      unsigned offset)
    {
        Lexer lex(sm.getLocForStartOfFile(sm.getMainFileID()), lo, src.begin(),
                  src.begin() + offset, src.end());
        Token tok;
        lex.LexFromRawLexer(tok);
        return sm.getFileOffset(tok.getLocation());
    }

    // Match keyword locations with supported AST targets.
    void GenerationPass::collect_targets(ASTContext &ctx)
    {
        SourceManager &sm = ctx.getSourceManager();
        FileID main = sm.getMainFileID();

        FrontendCandidateCollector collector;
        for (Decl *d : ctx.getTranslationUnitDecl()->decls()) {
            if (sm.isInMainFile(sm.getExpansionLoc(d->getLocation()))) {
                collector.TraverseDecl(d);
            }
        }

        auto mainOffset = [&](SourceLocation loc) -> std::optional<unsigned> {
            if (loc.isInvalid()) {
                return std::nullopt;
            }
            auto [FID, Offset] = sm.getDecomposedLoc(sm.getExpansionLoc(loc));
            if (FID != main) {
                return std::nullopt;
            }
            return Offset;
        };

        std::set<const void *> claimed;
        for (SourceLocation kw : m_keywords) {
            if (kw.isMacroID()) {
                report(sm.getExpansionLoc(kw), DiagnosticsEngine::Error,
                       "__llm__ cannot be used inside a macro expansion");
                continue;
            }
            if (!sm.isInMainFile(kw)) {
                report(kw, DiagnosticsEngine::Error,
                       "__llm__ function in included header '%0'; llmc++ only rewrites "
                       "the main file")
                    << llvm::sys::path::filename(sm.getFilename(kw));
                continue;
            }

            unsigned kwOffset = sm.getFileOffset(kw);
            data::DataGenerationTarget t;
            t.m_keyword_offset = kwOffset;
            t.m_options = m_opts;
            t.m_cache_salt = m_opts.m_cache_salt;
            std::string optionError;
            if (!parse_target_options(m_state.m_source, t, optionError)) {
                report(kw, DiagnosticsEngine::Error, "%0") << optionError;
                continue;
            }
            unsigned next =
                next_token_offset(sm, ctx.getLangOpts(), m_state.m_source, t.m_keyword_end);
            for (LambdaExpr *le : collector.m_lambdas) {
                if (mainOffset(le->getBeginLoc()) == next) {
                    t.m_lambda = le;
                    t.m_function = le->getCallOperator();
                    break;
                }
            }
            if (!t.m_lambda) {
                FunctionDecl *exact = nullptr, *around = nullptr;
                for (FunctionDecl *fd : collector.m_functions) {
                    std::optional<unsigned> begin = mainOffset(fd->getBeginLoc());
                    std::optional<unsigned> name = mainOffset(fd->getLocation());
                    std::optional<unsigned> templateBegin;
                    if (FunctionTemplateDecl *ft = fd->getDescribedFunctionTemplate()) {
                        templateBegin = mainOffset(ft->getBeginLoc());
                    }
                    if (begin == next || (templateBegin && *templateBegin == next)) {
                        exact = fd;
                        break;
                    }
                    if (begin && name && *begin < kwOffset && kwOffset < *name) {
                        around = fd;
                    }
                }
                t.m_function = exact ? exact : around;
            }
            if (!t.m_function) {
                report(kw, DiagnosticsEngine::Error,
                       "__llm__ must be followed by a function definition or a lambda");
                continue;
            }
            const void *key = t.m_lambda ? static_cast<const void *>(t.m_lambda)
                                         : static_cast<const void *>(t.m_function);
            if (!claimed.insert(key).second) {
                report(kw, DiagnosticsEngine::Error, "duplicate __llm__");
                continue;
            }
            if (check_target(ctx, t, kw)) {
                m_state.m_targets.push_back(std::move(t));
            }
        }

        llvm::sort(m_state.m_targets,
                   [](const data::DataGenerationTarget &a, const data::DataGenerationTarget &b) {
                       return a.m_l_brace < b.m_l_brace;
                   });
        name_targets(ctx);
    }

    // Validate a target and record its prompt source range.
    bool GenerationPass::check_target(ASTContext &ctx, data::DataGenerationTarget &t,
                                      SourceLocation kw)
    {
        SourceManager &sm = ctx.getSourceManager();
        FunctionDecl *fd = t.m_function;
        std::string what = t.m_lambda ? "lambda" : "'" + fd->getQualifiedNameAsString() + "'";
        Stmt *bodyStmt = t.m_lambda ? t.m_lambda->getBody() : fd->getBody();
        if (!t.m_lambda) {
            if (fd->isDefaulted() || fd->isDeleted()) {
                report(kw, DiagnosticsEngine::Error,
                       "__llm__ function %0 cannot be defaulted or deleted")
                    << what;
                return false;
            }
            if (!fd->doesThisDeclarationHaveABody()) {
                report(kw, DiagnosticsEngine::Error,
                       "__llm__ function %0 must have a body containing the prompt")
                    << what;
                return false;
            }
            if (fd->getConstexprKind() != ConstexprSpecKind::Unspecified) {
                report(kw, DiagnosticsEngine::Error, "__llm__ function %0 cannot be constexpr")
                    << what;
                return false;
            }
        }

        if (isa_and_nonnull<CXXTryStmt>(bodyStmt)) {
            report(kw, DiagnosticsEngine::Error,
                   "__llm__ function %0 cannot have a function-try-block")
                << what;
            return false;
        }
        auto *body = dyn_cast_or_null<CompoundStmt>(bodyStmt);
        if (!body) {
            report(kw, DiagnosticsEngine::Error,
                   "__llm__ function %0 must have a body containing the prompt")
                << what;
            return false;
        }
        SourceLocation l = body->getLBracLoc(), r = body->getRBracLoc();
        if (l.isMacroID() || r.isMacroID() || !sm.isInMainFile(l)) {
            report(kw, DiagnosticsEngine::Error,
                   "the body of __llm__ function %0 must be written in the main file")
                << what;
            return false;
        }
        t.m_body = body;
        t.m_l_brace = sm.getFileOffset(l);
        t.m_r_brace = sm.getFileOffset(r);

        StringRef src = m_state.m_source;
        StringRef bodyText = src.slice(t.m_l_brace + 1, t.m_r_brace);
        unsigned lineOffset = 0;
        while (lineOffset < bodyText.size()) {
            StringRef rest = bodyText.drop_front(lineOffset);
            StringRef lineText = rest.split('\n').first;
            size_t hash = lineText.find_first_not_of(" \t\r");
            if (hash != StringRef::npos && lineText[hash] == '#') {
                SourceLocation hashLoc = sm.getLocForStartOfFile(sm.getMainFileID())
                                             .getLocWithOffset(t.m_l_brace + 1 + lineOffset + hash);
                report(hashLoc, DiagnosticsEngine::Error,
                       "preprocessor directives are not allowed in an __llm__ function "
                       "body");
                return false;
            }
            size_t newline = rest.find('\n');
            if (newline == StringRef::npos) {
                break;
            }
            lineOffset += newline + 1;
        }

        Lexer lex(sm.getLocForStartOfFile(sm.getMainFileID()), ctx.getLangOpts(), src.begin(),
                  src.begin() + t.m_l_brace + 1, src.end());
        lex.SetCommentRetentionState(true);
        Token tok;
        std::vector<std::pair<unsigned, unsigned>> comments;
        while (true) {
            lex.LexFromRawLexer(tok);
            unsigned offset = sm.getFileOffset(tok.getLocation());
            if (tok.is(tok::eof) || offset >= t.m_r_brace) {
                break;
            }
            if (tok.is(tok::comment)) {
                comments.push_back({offset, offset + tok.getLength()});
            }
        }

        std::string prompt = bodyText.str();
        for (const auto &[begin, end] : comments) {
            for (unsigned i = begin - t.m_l_brace - 1; i < end - t.m_l_brace - 1; ++i) {
                if (prompt[i] != '\n' && prompt[i] != '\r') {
                    prompt[i] = ' ';
                }
            }
        }
        t.m_prompt_text = dedent(prompt);

        PresumedLoc p = sm.getPresumedLoc(kw);
        t.m_location = formatv("{0}:{1}:{2}", llvm::sys::path::filename(p.getFilename()),
                               p.getLine(), p.getColumn())
                           .str();
        return true;
    }

    // Fingerprint source context without output paths or compilation actions.
    std::string GenerationPass::context_digest() const
    {
        std::string context =
            getClangFullVersion() + "\n" + make_parseable_source(m_state.m_source);
        bool valueFollows = false;
        for (const std::string &arg : m_cc1_args) {
            StringRef option(arg);
            if (valueFollows || option.starts_with("-std=") || option.starts_with("-D") ||
                option.starts_with("-U") || option.starts_with("-I") || option.starts_with("-f") ||
                option.starts_with("-m") || option.starts_with("-O") || option == "-pthread" ||
                option == "-triple" || option == "-target-cpu" || option == "-target-feature" ||
                option == "-isystem" || option == "-iquote" || option == "-include" ||
                option == "-x") {
                if (option != "-fsyntax-only") {
                    context += "\nargument:" + arg;
                }
            }
            valueFollows = option == "-D" || option == "-U" || option == "-I" ||
                           option == "-triple" || option == "-target-cpu" ||
                           option == "-target-feature" || option == "-isystem" ||
                           option == "-iquote" || option == "-include" || option == "-x";
        }
        std::set<std::string> headers;
        for (const auto &include : m_state.m_includes) {
            if (!include.m_path.empty() && headers.insert(include.m_path).second) {
                auto buffer = llvm::MemoryBuffer::getFile(include.m_path);
                if (buffer) {
                    context += "\nheader:" + include.m_path + "\n" + (*buffer)->getBuffer().str();
                }
            }
        }
        return sha256_hex(context);
    }

    // Assign stable display names and cache keys to targets.
    void GenerationPass::name_targets(ASTContext &ctx)
    {
        std::string contextDigest = context_digest();
        llvm::StringMap<unsigned> lambdaCounts;
        for (data::DataGenerationTarget &t : m_state.m_targets) {
            if (t.m_lambda) {
                std::string enclosing;
                for (const DeclContext *dc = t.m_lambda->getLambdaClass()->getDeclContext(); dc;
                     dc = dc->getParent()) {
                    if (const auto *nd = dyn_cast<NamedDecl>(dc)) {
                        enclosing = nd->getQualifiedNameAsString();
                        break;
                    }
                }
                unsigned n = ++lambdaCounts[enclosing];
                t.m_name = (enclosing.empty() ? "" : enclosing + "::") + "(lambda #" +
                           std::to_string(n) + ")";
                t.m_signature = lambda_signature(t.m_lambda, ctx);
            } else {
                t.m_name = t.m_function->getQualifiedNameAsString();
                t.m_signature = function_signature(t.m_function, ctx);
            }
            t.m_context_digest = contextDigest;
            std::string policy =
                formatv("{0}", json::Value(agent_generation_settings(t.m_options))).str();
            t.m_key =
                sha256_hex(std::string(CacheVersion) + "\n" + t.m_name + "\n" + t.m_signature +
                           "\n" + t.m_prompt_text + "\n" + policy + "\n" + t.m_cache_salt + "\n" +
                           sha256_hex(t.m_options.m_system_prompt) + "\n" +
                           sha256_hex(t.m_options.m_agent_config) + "\n" + t.m_options.m_backend +
                           "\n" + contextDigest +
                           (!t.m_agent_override ? "" : "\nagent:" + t.m_options.m_agent_command));
            if (!t.m_cache_key.empty()) {
                t.m_key = t.m_cache_key;
            }
        }
    }

    // Resolve a target's keyword source location.
    SourceLocation GenerationPass::keyword_loc(const data::DataGenerationTarget &t) const
    {
        SourceManager &sm = m_ci.getSourceManager();
        return sm.getLocForStartOfFile(sm.getMainFileID()).getLocWithOffset(t.m_keyword_offset);
    }

    // Generate and validate an implementation for one target.
    bool GenerationPass::generate(data::DataGenerationTarget &t)
    {
        if (t.m_options.m_force_regenerate) {
            t.m_options.m_offline = false;
        }
        if (t.m_options.m_use_cache && !t.m_options.m_force_regenerate && read_cache(t)) {
            t.m_generated = true;
            if (t.m_options.m_verbose) {
                llvm::errs() << "llmc++: " << t.m_location << ": '" << t.m_name << "' from "
                             << cache_path(t) << "\n";
            }
            dump(t);
            return true;
        }
        if (m_ci.getDiagnostics().hasErrorOccurred()) {
            return false;
        }
        if (t.m_options.m_offline) {
            report(keyword_loc(t), DiagnosticsEngine::Error,
                   "no cached body for __llm__ function '%0' (%1)")
                << t.m_name << (m_opts.m_offline ? "-fllm-offline" : "offline");
            return false;
        }

        if (t.m_options.m_verbose) {
            llvm::errs() << "llmc++: " << t.m_location << ": generating '" << t.m_name << "' ...\n";
        }
        auto start = std::chrono::steady_clock::now();
        AgentToolServer tools(m_state, t);
        data::DataAgentOutcome out;
        std::string problem;
        json::Object task = tools.get_task();
        task["protocol_version"] = 1;
        task["capabilities"] = json::Array{"compiler_tools", "effective_settings"};
        task["system_prompt"] = t.m_options.m_system_prompt;
        task["context_digest"] = t.m_context_digest;
        auto config = json::parse(t.m_options.m_agent_config);
        task["agent_config"] = std::move(*config);
        AgentSession agent(t.m_options);
        agent_record(t.m_options, "generation", json::Object(task));
        if (t.m_options.m_verbose) {
            llvm::errs() << "llmc++: system prompt " << sha256_hex(t.m_options.m_system_prompt)
                         << ", agent " << agent_identity(t.m_options) << "\n";
        }
        bool talked = agent.generate(std::move(task), tools, out, problem);
        agent_record(t.m_options, "outcome",
                     json::Object{{"name", t.m_name},
                                  {"status", out.m_status},
                                  {"model", out.m_model},
                                  {"message", out.m_message},
                                  {"error", problem}});
        double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        if (!tools.accepted() || !talked || out.m_status == "error") {
            std::string why = !talked                  ? problem
                              : !out.m_message.empty() ? out.m_message
                                                       : "the agent finished without an "
                                                         "accepted submit";
            report(keyword_loc(t), DiagnosticsEngine::Error,
                   "LLM failed to generate a body for '%0': %1")
                << t.m_name << why;
            if (!tools.last_attempt().empty()) {
                report(keyword_loc(t), DiagnosticsEngine::Note, "last rejected attempt:\n%0\n%1")
                    << tools.last_attempt() << tools.last_diagnostics();
            }
            return false;
        }

        t.m_code = tools.accepted_body();
        t.m_model = !out.m_model.empty()     ? out.m_model
                    : !agent.model().empty() ? agent.model()
                                             : "unknown";
        t.m_generated = true;
        t.m_date = current_time();
        t.m_agent_identity = agent_identity(t.m_options);
        if (t.m_options.m_use_cache) {
            write_cache(t);
        }
        if (t.m_options.m_verbose) {
            llvm::errs() << formatv("llmc++: {0}: generated '{1}' in {2:f1}s ({3}, "
                                    "{4} tool call{5})\n",
                                    t.m_location, t.m_name, seconds, t.m_model, out.m_tool_calls,
                                    out.m_tool_calls == 1 ? "" : "s");
        }
        dump(t);
        return true;
    }

    // Print a target's generated implementation for diagnostics.
    void GenerationPass::dump(const data::DataGenerationTarget &t) const
    {
        if (t.m_options.m_dump) {
            llvm::errs() << "llmc++: body of '" << t.m_name << "':\n"
                         << reindent(t.m_code, "    ") << "\n";
        }
    }

    // Apply generated bodies and remove keyword spellings.
    std::string GenerationPass::rewrite() const
    {
        StringRef src = m_state.m_source;
        std::vector<data::DataSourceEdit> edits;
        for (const data::DataGenerationTarget &t : m_state.m_targets) {
            unsigned end = t.m_keyword_end;
            while (end < src.size() && (src[end] == ' ' || src[end] == '\t')) {
                ++end;
            }
            edits.push_back({t.m_keyword_offset, end, ""});

            std::string close = (first_on_line(src, t.m_r_brace) ? line_indent(src, t.m_r_brace)
                                                                 : line_indent(src, t.m_l_brace))
                                    .str();
            std::string indent = close + "    ";
            std::string text = "\n" + reindent(annotated_body(t, true), indent) + close + "}";
            edits.push_back({t.m_l_brace + 1, t.m_r_brace + 1, std::move(text)});
        }
        return apply_edits(src, edits);
    }

    // Resolve the cache directory for the current source file.
    std::string GenerationPass::cache_dir(const data::DataGenerationTarget &t) const
    {
        if (!t.m_options.m_cache_dir.empty()) {
            return t.m_options.m_cache_dir;
        }
        llvm::SmallString<256> dir(m_state.m_main_file);
        llvm::sys::fs::make_absolute(dir);
        llvm::sys::path::remove_filename(dir);
        llvm::sys::path::append(dir, ".llmcache");
        return std::string(dir);
    }

    // Resolve the cache file for a target.
    std::string GenerationPass::cache_path(const data::DataGenerationTarget &t) const
    {
        llvm::SmallString<256> path(cache_dir(t));
        llvm::sys::path::append(path, abbreviate(t.m_key, t) + ".cpp");
        return std::string(path);
    }

    // Extend digest prefixes when known targets or cache entries would be ambiguous.
    std::string GenerationPass::abbreviate(StringRef digest,
                                           const data::DataGenerationTarget &t) const
    {
        size_t length = std::min<size_t>(m_opts.m_hash_abbrev, digest.size());
        auto distinguish = [&](StringRef other) {
            if (other == digest) {
                return;
            }
            while (length < digest.size() && other.starts_with(digest.take_front(length))) {
                ++length;
            }
        };
        for (const auto &target : m_state.m_targets) {
            distinguish(target.m_key);
            distinguish(target.m_context_digest);
            distinguish(sha256_hex(target.m_options.m_system_prompt));
            distinguish(sha256_hex(target.m_options.m_agent_config));
        }
        std::error_code error;
        for (llvm::sys::fs::directory_iterator entry(cache_dir(t), error), end;
             !error && entry != end; entry.increment(error)) {
            StringRef path = entry->path();
            if (llvm::sys::path::extension(path) != ".cpp") {
                continue;
            }
            auto buffer = llvm::MemoryBuffer::getFile(path);
            bool ownEntry =
                buffer && (*buffer)->getBuffer().contains("// key: " + digest.str() + "\n");
            if (!ownEntry) {
                distinguish(llvm::sys::path::stem(path));
            }
            if (buffer) {
                llvm::SmallVector<StringRef, 32> lines;
                (*buffer)->getBuffer().split(lines, '\n');
                for (StringRef line : lines) {
                    if (line.consume_front("// key: ") || line.consume_front("// context: ") ||
                        line.consume_front("// system_prompt: ") ||
                        line.consume_front("// agent_config: ")) {
                        distinguish(line.trim());
                    }
                }
            }
        }
        return digest.take_front(length).str();
    }

    // Share the metadata layout between cache entries and generated source bodies.
    std::string GenerationPass::annotated_body(const data::DataGenerationTarget &t,
                                               bool abbreviated) const
    {
        auto hash = [&](StringRef digest) {
            return abbreviated ? abbreviate(digest, t) : digest.str();
        };
        std::string body;
        llvm::raw_string_ostream os(body);
        os << "// version: " << CacheVersion << "\n"
           << "// key: " << hash(t.m_key) << "\n"
           << "// context: " << hash(t.m_context_digest) << "\n"
           << "// system_prompt: " << hash(sha256_hex(t.m_options.m_system_prompt)) << "\n"
           << "// policy: " << formatv("{0}", json::Value(agent_generation_settings(t.m_options)))
           << "\n"
           << "// cache_salt: " << formatv("{0}", json::Value(t.m_cache_salt)) << "\n"
           << "// agent_config: " << hash(sha256_hex(t.m_options.m_agent_config)) << "\n"
           << "// agent: " << t.m_agent_identity << "\n"
           << "// function: " << t.m_signature << "\n"
           << "// location: " << t.m_location << "\n"
           << "// model: " << t.m_model << "\n"
           << "// date: " << t.m_date << "\n"
           << "// prompt:\n";
        llvm::SmallVector<StringRef, 16> promptLines;
        StringRef(t.m_prompt_text).split(promptLines, '\n');
        for (StringRef line : promptLines) {
            os << "//   " << line << "\n";
        }
        os << "// ---\n" << t.m_code;
        if (!t.m_code.empty() && t.m_code.back() != '\n') {
            os << "\n";
        }
        return body;
    }

    // Load a compatible generated implementation from cache.
    bool GenerationPass::read_cache(data::DataGenerationTarget &t)
    {
        std::string path = cache_path(t);
        std::string matchedKey;
        std::error_code error;
        for (llvm::sys::fs::directory_iterator entry(cache_dir(t), error), end;
             !error && entry != end; entry.increment(error)) {
            if (llvm::sys::path::extension(entry->path()) != ".cpp") {
                continue;
            }
            auto candidate = llvm::MemoryBuffer::getFile(entry->path());
            if (!candidate) {
                continue;
            }
            StringRef content = (*candidate)->getBuffer();
            size_t keyBegin = content.find("// key: ");
            if (keyBegin == StringRef::npos) {
                continue;
            }
            StringRef key = content.drop_front(keyBegin + 8).split('\n').first;
            if (key == t.m_key || (!t.m_cache_key.empty() && key.starts_with(t.m_cache_key))) {
                if (!matchedKey.empty() && matchedKey != key) {
                    report(keyword_loc(t), DiagnosticsEngine::Error,
                           "ambiguous cache key '%0'; use a longer hash")
                        << t.m_cache_key;
                    return false;
                }
                matchedKey = key.str();
                path = entry->path();
            }
        }
        if (t.m_options.m_cache_lifetime) {
            llvm::sys::fs::file_status status;
            if (llvm::sys::fs::status(path, status) ||
                std::chrono::system_clock::now() - status.getLastModificationTime() >
                    std::chrono::seconds(t.m_options.m_cache_lifetime)) {
                return false;
            }
        }
        auto buf = llvm::MemoryBuffer::getFile(path, true);
        if (!buf) {
            return false;
        }
        StringRef content = (*buf)->getBuffer();
        const StringRef separator = "\n// ---\n";
        size_t sep = content.find(separator);
        if (sep == StringRef::npos) {
            return false;
        }
        StringRef metadata = content.take_front(sep);
        if (!metadata.contains("// version: " + std::string(CacheVersion) + "\n") ||
            !metadata.contains("// key: " + (matchedKey.empty() ? t.m_key : matchedKey) + "\n")) {
            return false;
        }
        if (t.m_cache_key.empty() &&
            (!metadata.contains("// context: " + t.m_context_digest + "\n") ||
             !metadata.contains("// system_prompt: " + sha256_hex(t.m_options.m_system_prompt) +
                                "\n"))) {
            return false;
        }
        if (!matchedKey.empty()) {
            t.m_key = matchedKey;
        }
        llvm::SmallVector<StringRef, 16> lines;
        content.substr(0, sep).split(lines, '\n');
        t.m_model = "unknown";
        for (StringRef l : lines) {
            if (l.consume_front("// model: ")) {
                t.m_model = l.trim().str();
            } else if (l.consume_front("// date: ")) {
                t.m_date = l.trim().str();
            } else if (l.consume_front("// agent: ")) {
                t.m_agent_identity = l.trim().str();
            }
        }
        t.m_code = content.substr(sep + separator.size()).str();
        return true;
    }

    // Store a generated implementation and its metadata atomically.
    void GenerationPass::write_cache(const data::DataGenerationTarget &t)
    {
        std::string dir = cache_dir(t);
        if (std::error_code ec = llvm::sys::fs::create_directories(dir)) {
            report(SourceLocation(), DiagnosticsEngine::Warning,
                   "cannot create cache directory '%0': %1")
                << dir << ec.message();
            return;
        }
        std::string path = cache_path(t);
        llvm::SmallString<256> temp;
        int descriptor = -1;
        if (std::error_code ec =
                llvm::sys::fs::createUniqueFile(path + ".%%%%%%.tmp", descriptor, temp)) {
            report(SourceLocation(), DiagnosticsEngine::Warning, "cannot create cache entry: %0")
                << ec.message();
            return;
        }
        {
            llvm::raw_fd_ostream os(descriptor, true);
            os << annotated_body(t, false);
            os.flush();
            if (os.has_error()) {
                report(SourceLocation(), DiagnosticsEngine::Warning,
                       "cannot write cache entry '%0'")
                    << path;
                os.clear_error();
                llvm::sys::fs::remove(temp);
                return;
            }
        }
        if (std::error_code ec = llvm::sys::fs::rename(temp, path)) {
            llvm::sys::fs::remove(temp);
            report(SourceLocation(), DiagnosticsEngine::Warning, "cannot publish cache entry: %0")
                << ec.message();
            return;
        }
        std::error_code error;
        for (llvm::sys::fs::directory_iterator entry(dir, error), end; !error && entry != end;
             entry.increment(error)) {
            if (entry->path() == path || llvm::sys::path::extension(entry->path()) != ".cpp") {
                continue;
            }
            auto previous = llvm::MemoryBuffer::getFile(entry->path());
            if (previous && (*previous)->getBuffer().contains("// key: " + t.m_key + "\n")) {
                llvm::sys::fs::remove(entry->path());
            }
        }
    }

}
