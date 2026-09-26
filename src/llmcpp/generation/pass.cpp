/*
 * C++ file for prompt discovery, generation, caching, and source rewriting.
 */

// Project headers for pass state, tools, transport, and text handling.
#include "llmcpp/generation/pass.h"
#include "llmcpp/agent/agent_session.h"
#include "llmcpp/agent/tool_server.h"
#include "llmcpp/compiler/ast_text.h"
#include "llmcpp/compiler/shadow_compiler.h"
#include "llmcpp/data/source_token.h"
#include "llmcpp/frontend/callbacks.h"
#include "llmcpp/frontend/candidate_collector.h"
#include "llmcpp/frontend/pass_action.h"
#include "llmcpp/generation/pass_state.h"
#include "llmcpp/source/text.h"

// Clang headers for AST traversal, preprocessing, and frontend execution.
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
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
    // Namespace for llmcpp generation implementation.
    namespace generation
    {
        // Namespace for pass implementation details private to this translation unit.
        namespace
        {

            // Constants for lexical markers used while discovering prompt bodies.
            const char Keyword[] = "__llm__";
            const unsigned KeywordLength = sizeof(Keyword) - 1;
            const char CacheVersion[] = "llmcpp-prototype-2";

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
            static std::vector<data::SourceToken> source_tokens(StringRef source)
            {
                std::vector<data::SourceToken> tokens;
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
                        while (i + 1 < source.size() &&
                               !(source[i] == '*' && source[i + 1] == '/')) {
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
                    tokens.push_back({i, i + 1, directive});
                    ++i;
                }
                return tokens;
            }

            // Compare a source token with expected spelling.
            static bool token_is(StringRef source, const data::SourceToken &t, StringRef text)
            {
                return source.slice(t.m_begin, t.m_end) == text;
            }

            // Find the closing brace paired with a prompt body's opening brace.
            static std::optional<unsigned>
            matching_prompt_brace(StringRef source, llvm::ArrayRef<data::SourceToken> tokens,
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
                std::vector<data::SourceToken> tokens = source_tokens(source);
                std::string result = source.str();
                for (size_t i = 0; i < tokens.size();) {
                    if (tokens[i].m_directive || !token_is(source, tokens[i], Keyword)) {
                        ++i;
                        continue;
                    }

                    unsigned parens = 0, squares = 0;
                    bool sawParameterList = false, inCtorInitializers = false;
                    size_t bodyOpen = tokens.size();
                    unsigned bodyCloseOffset = 0;
                    for (size_t j = i + 1; j < tokens.size(); ++j) {
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
                                    StringRef next =
                                        k < tokens.size()
                                            ? source.slice(tokens[k].m_begin, tokens[k].m_end)
                                            : StringRef();
                                    if (next == "," || next == "{") {
                                        j = close;
                                        continue;
                                    }
                                }
                            }

                            bodyOpen = j;
                            std::optional<unsigned> close =
                                matching_prompt_brace(source, tokens, j);
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
        Pass::Pass(CompilerInstance &ci, const data::Options &opts,
                   std::vector<std::string> cc1Args, std::string originalSource,
                   data::PassResult &result)
            : m_state(ci, opts)
            , m_ci(ci)
            , m_opts(opts)
            , m_result(result)
            , m_agent(opts)
            , m_cc1_args(std::move(cc1Args))
            , m_original_source(std::move(originalSource))
        {
            // Empty.
        }

        // Return annotation locations collected during preprocessing.
        std::vector<SourceLocation> &Pass::keyword_locations()
        {
            return m_keywords;
        }

        // Return shared state for the active compilation pass.
        PassState &Pass::state()
        {
            return m_state;
        }

        // Emit a pass diagnostic using a compile-time format string.
        template <unsigned N>
        DiagnosticBuilder Pass::report(SourceLocation location, DiagnosticsEngine::Level level,
                                       const char (&format)[N])
        {
            DiagnosticsEngine &diagnostics = m_ci.getDiagnostics();
            return diagnostics.Report(location, diagnostics.getCustomDiagID(level, format));
        }

        // Run generation and rewriting after parsing succeeds.
        void Pass::run(ASTContext &ctx)
        {
            DiagnosticsEngine &diags = ctx.getDiagnostics();
            if (diags.hasErrorOccurred()) {
                m_result.m_status = data::PassStatus::Failed;
                return;
            }
            m_state.m_source = m_original_source;
            if (m_keywords.empty()) {
                return;
            }

            collect_targets(ctx);
            if (diags.hasErrorOccurred()) {
                m_result.m_status = data::PassStatus::Failed;
                return;
            }
            if (m_state.m_targets.empty()) {
                return;
            }

            m_state.m_shadow = std::make_unique<compiler::ShadowCompiler>(
                m_cc1_args, m_state.m_main_file, m_opts.m_executable);

            if (m_opts.m_dump_context) {
                for (data::Target &t : m_state.m_targets) {
                    agent::ToolServer tools(m_state, t);
                    json::Object o{{"task", tools.get_task()}, {"context", tools.get_context()}};
                    llvm::outs() << formatv("{0:2}", json::Value(std::move(o))) << "\n";
                }
                m_result.m_status = data::PassStatus::DumpedContext;
                return;
            }

            bool ok = true;
            for (data::Target &t : m_state.m_targets) {
                ok &= generate(t);
            }
            if (!ok) {
                m_result.m_status = data::PassStatus::Failed;
                return;
            }

            std::string output = rewrite();
            if (m_opts.m_emit_source) {
                data::ShadowResult r = m_state.m_shadow->compile(
                    output, 0, 0, false, llvm::sys::path::filename(m_state.m_main_file));
                if (!r.m_ok) {
                    report(SourceLocation(), DiagnosticsEngine::Error,
                           "the rewritten '%0' does not compile:\n%1")
                        << llvm::sys::path::filename(m_state.m_main_file) << r.m_text;
                    m_result.m_status = data::PassStatus::Failed;
                    return;
                }
            }
            m_result.m_status = data::PassStatus::Rewritten;
            m_result.m_output = std::move(output);
        }

        // Find the next raw token at or after a source offset.
        static unsigned next_token_offset(const SourceManager &sm, const LangOptions &lo,
                                          StringRef src, unsigned offset)
        {
            Lexer lex(sm.getLocForStartOfFile(sm.getMainFileID()), lo, src.begin(),
                      src.begin() + offset, src.end());
            Token tok;
            lex.LexFromRawLexer(tok);
            return sm.getFileOffset(tok.getLocation());
        }

        // Match keyword locations with supported AST targets.
        void Pass::collect_targets(ASTContext &ctx)
        {
            SourceManager &sm = ctx.getSourceManager();
            FileID main = sm.getMainFileID();

            frontend::CandidateCollector collector;
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
                unsigned next = next_token_offset(sm, ctx.getLangOpts(), m_state.m_source,
                                                  kwOffset + KeywordLength);
                data::Target t;
                t.m_keyword_offset = kwOffset;
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

            llvm::sort(m_state.m_targets, [](const data::Target &a, const data::Target &b) {
                return a.m_l_brace < b.m_l_brace;
            });
            name_targets(ctx);
        }

        // Validate a target and record its prompt source range.
        bool Pass::check_target(ASTContext &ctx, data::Target &t, SourceLocation kw)
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
                    SourceLocation hashLoc =
                        sm.getLocForStartOfFile(sm.getMainFileID())
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
            t.m_prompt_text = source::dedent(prompt);

            PresumedLoc p = sm.getPresumedLoc(kw);
            t.m_location = formatv("{0}:{1}:{2}", llvm::sys::path::filename(p.getFilename()),
                                   p.getLine(), p.getColumn())
                               .str();
            return true;
        }

        // Assign stable display names and cache keys to targets.
        void Pass::name_targets(ASTContext &ctx)
        {
            llvm::StringMap<unsigned> lambdaCounts;
            for (data::Target &t : m_state.m_targets) {
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
                    t.m_signature = compiler::lambda_signature(t.m_lambda, ctx);
                } else {
                    t.m_name = t.m_function->getQualifiedNameAsString();
                    t.m_signature = compiler::function_signature(t.m_function, ctx);
                }
                t.m_key = source::sha256_hex(std::string(CacheVersion) + "\n" + t.m_name + "\n" +
                                             t.m_signature + "\n" + t.m_prompt_text)
                              .substr(0, 24);
            }
        }

        // Resolve a target's keyword source location.
        SourceLocation Pass::keyword_loc(const data::Target &t) const
        {
            SourceManager &sm = m_ci.getSourceManager();
            return sm.getLocForStartOfFile(sm.getMainFileID()).getLocWithOffset(t.m_keyword_offset);
        }

        // Generate and validate an implementation for one target.
        bool Pass::generate(data::Target &t)
        {
            if (m_opts.m_use_cache && !m_opts.m_regenerate && read_cache(t)) {
                t.m_generated = true;
                if (m_opts.m_verbose) {
                    llvm::errs() << "llmc++: " << t.m_location << ": '" << t.m_name << "' from "
                                 << cache_path(t) << "\n";
                }
                dump(t);
                return true;
            }
            if (m_opts.m_offline) {
                report(keyword_loc(t), DiagnosticsEngine::Error,
                       "no cached body for __llm__ function '%0' (-fllm-offline)")
                    << t.m_name;
                return false;
            }

            if (!m_opts.m_quiet) {
                llvm::errs() << "llmc++: " << t.m_location << ": generating '" << t.m_name
                             << "' ...\n";
            }
            auto start = std::chrono::steady_clock::now();
            agent::ToolServer tools(m_state, t);
            data::AgentOutcome out;
            std::string problem;
            json::Object task{
                {"name", t.m_name},
                {"signature", t.m_signature},
                {"location", t.m_location},
                {"prompt", t.m_prompt_text},
                {"limits", json::Object{{"max_attempts", m_opts.m_max_attempts},
                                        {"max_tool_calls", m_opts.m_max_tool_calls},
                                        {"timeout_seconds", m_opts.m_timeout_seconds}}}};
            bool talked = m_agent.generate(std::move(task), tools, out, problem);
            double seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

            if (!tools.accepted()) {
                std::string why = !talked                  ? problem
                                  : !out.m_message.empty() ? out.m_message
                                                           : "the agent finished without an "
                                                             "accepted submit";
                report(keyword_loc(t), DiagnosticsEngine::Error,
                       "LLM failed to generate a body for '%0': %1")
                    << t.m_name << why;
                if (!tools.last_attempt().empty()) {
                    report(keyword_loc(t), DiagnosticsEngine::Note,
                           "last rejected attempt:\n%0\n%1")
                        << tools.last_attempt() << tools.last_diagnostics();
                }
                return false;
            }

            t.m_code = tools.accepted_body();
            t.m_model = !out.m_model.empty()       ? out.m_model
                        : !m_agent.model().empty() ? m_agent.model()
                                                   : "unknown";
            t.m_generated = true;
            if (m_opts.m_use_cache) {
                write_cache(t);
            }
            if (!m_opts.m_quiet) {
                llvm::errs() << formatv("llmc++: {0}: generated '{1}' in {2:f1}s ({3}, "
                                        "{4} tool call{5})\n",
                                        t.m_location, t.m_name, seconds, t.m_model,
                                        out.m_tool_calls, out.m_tool_calls == 1 ? "" : "s");
            }
            dump(t);
            return true;
        }

        // Print a target's generated implementation for diagnostics.
        void Pass::dump(const data::Target &t) const
        {
            if (m_opts.m_dump) {
                llvm::errs() << "llmc++: body of '" << t.m_name << "':\n"
                             << source::reindent(t.m_code, "    ") << "\n";
            }
        }

        // Apply generated bodies and remove keyword spellings.
        std::string Pass::rewrite() const
        {
            StringRef src = m_state.m_source;
            std::vector<data::Edit> edits;
            for (const data::Target &t : m_state.m_targets) {
                unsigned end = t.m_keyword_offset + KeywordLength;
                while (end < src.size() && (src[end] == ' ' || src[end] == '\t')) {
                    ++end;
                }
                edits.push_back({t.m_keyword_offset, end, ""});

                std::string close = (source::first_on_line(src, t.m_r_brace)
                                         ? source::line_indent(src, t.m_r_brace)
                                         : source::line_indent(src, t.m_l_brace))
                                        .str();
                std::string indent = close + "    ";
                std::string text = "\n";
                llvm::SmallVector<StringRef, 16> promptLines;
                StringRef(t.m_prompt_text).split(promptLines, '\n');
                for (StringRef line : promptLines) {
                    text += indent + "//";
                    if (!line.empty()) {
                        text += " " + line.str();
                    }
                    text += "\n";
                }
                text += indent + "// llmcpp: generated (model=" + t.m_model + ", key=" + t.m_key +
                        ")\n" + source::reindent(t.m_code, indent) + close + "}";
                edits.push_back({t.m_l_brace + 1, t.m_r_brace + 1, std::move(text)});
            }
            return source::apply_edits(src, edits);
        }

        // Resolve the cache directory for the current source file.
        std::string Pass::cache_dir() const
        {
            if (!m_opts.m_cache_dir.empty()) {
                return m_opts.m_cache_dir;
            }
            llvm::SmallString<256> dir(m_state.m_main_file);
            llvm::sys::fs::make_absolute(dir);
            llvm::sys::path::remove_filename(dir);
            llvm::sys::path::append(dir, ".llmcache");
            return std::string(dir);
        }

        // Resolve the cache file for a target.
        std::string Pass::cache_path(const data::Target &t) const
        {
            llvm::SmallString<256> path(cache_dir());
            llvm::sys::path::append(path, t.m_key + ".cpp");
            return std::string(path);
        }

        // Load a compatible generated implementation from cache.
        bool Pass::read_cache(data::Target &t) const
        {
            auto buf = llvm::MemoryBuffer::getFile(cache_path(t), true);
            if (!buf) {
                return false;
            }
            StringRef content = (*buf)->getBuffer();
            const StringRef separator = "\n// ---\n";
            size_t sep = content.find(separator);
            if (sep == StringRef::npos) {
                return false;
            }
            llvm::SmallVector<StringRef, 16> lines;
            content.substr(0, sep).split(lines, '\n');
            t.m_model = "unknown";
            for (StringRef l : lines) {
                if (l.consume_front("// model: ")) {
                    t.m_model = l.trim().str();
                }
            }
            t.m_code = content.substr(sep + separator.size()).str();
            return true;
        }

        // Store a generated implementation and its metadata atomically.
        void Pass::write_cache(const data::Target &t)
        {
            std::string dir = cache_dir();
            if (std::error_code ec = llvm::sys::fs::create_directories(dir)) {
                report(SourceLocation(), DiagnosticsEngine::Warning,
                       "cannot create cache directory '%0': %1")
                    << dir << ec.message();
                return;
            }
            std::string path = cache_path(t);
            std::string temp = path + ".tmp";
            {
                std::error_code ec;
                llvm::raw_fd_ostream os(temp, ec, llvm::sys::fs::OF_Text);
                if (ec) {
                    report(SourceLocation(), DiagnosticsEngine::Warning,
                           "cannot write cache entry '%0': %1")
                        << temp << ec.message();
                    return;
                }
                os << "// llmcpp cache entry\n"
                   << "// function: " << t.m_signature << "\n"
                   << "// location: " << t.m_location << "\n"
                   << "// model: " << t.m_model << "\n"
                   << "// date: " << current_time() << "\n"
                   << "// prompt:\n";
                llvm::SmallVector<StringRef, 16> promptLines;
                StringRef(t.m_prompt_text).split(promptLines, '\n');
                for (StringRef l : promptLines) {
                    os << "//   " << l << "\n";
                }
                os << "// ---\n" << t.m_code;
                if (!t.m_code.empty() && t.m_code.back() != '\n') {
                    os << "\n";
                }
            }
            llvm::sys::fs::rename(temp, path);
        }

    }
}
