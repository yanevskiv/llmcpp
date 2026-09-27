/*
 * C++ file for Catch2 integration coverage for llmc++.
 */

// Catch2 declarations and integration-test support.
#include <catch2/catch_test_macros.hpp>

#include "llmcpp/test/test_fake_anthropic_server.h"
#include "llmcpp/test/test_fake_openai_server.h"
#include "llmcpp/test/test_text.h"
#include "llmcpp/test/test_workspace.h"

// Standard headers used by test scenarios.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Verify that driver help includes both Clang and llmcpp options without configuration.
TEST_CASE("driver help includes generation options", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *help : {"--help", "-help", "--help-hidden"}) {
        llmcpp::test::TestCommandResult result =
            work.llmcpp({help, "--llm", "-fllm-system-prompt=missing.md"},
                        {{"LLMCPP_BACKEND", ""}, {"LLMCPP_AGENT", ""}});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(result.m_err.empty());
        llmcpp::test::check_contains(result.m_out, {"USAGE:", "LLMCPP OPTIONS:", "--llm",
                                                    "-fllm-backend=", "-fllm-no-cache",
                                                    "-fllm-transcript=", "-fllm-verbose"});
    }
}

// Verify diagnostics for unsupported or malformed annotations.
TEST_CASE("invalid annotations produce llmc++ diagnostics", "[diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result = work.llmcpp({"-fsyntax-only", "test_errors.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(
        result.m_err, {"preprocessor directives are not allowed in an __llm__ function body",
                       "__llm__ function 'declaration_only' must have a body containing the prompt",
                       "__llm__ function 'compile_time' cannot be constexpr",
                       "__llm__ function 'try_block' cannot have a function-try-block",
                       "__llm__ function 'S::S' cannot be defaulted or deleted",
                       "__llm__ cannot be used inside a macro expansion",
                       "__llm__ must be followed by a function definition or a lambda"});
}

// Verify that annotations in included headers are rejected.
TEST_CASE("annotations in headers are rejected", "[diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *mode : {"-fsyntax-only", "--llm"}) {
        DYNAMIC_SECTION(mode)
        {
            llmcpp::test::TestCommandResult result = work.llmcpp({mode, "test_header.cpp"});
            REQUIRE(result.m_status != 0);
            llmcpp::test::check_contains(result.m_err,
                                         {"__llm__ function in included header 'test_header.h'"});
        }
    }
}

// Verify that offline mode fails when no cached body exists.
TEST_CASE("offline mode requires cached bodies", "[cache]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result = work.llmcpp(
        {"-fsyntax-only", "-fllm-offline", "-fllm-cache-dir=empty", "test_all_forms.cpp"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err,
                                 {"no cached body for __llm__ function 'sum' (-fllm-offline)"});
}

// Verify that plain prompt text reaches compiler-context output.
TEST_CASE("compiler context includes plain prompts", "[context]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-dump-context", "test_all_forms.cpp"});
    REQUIRE(result.m_status == 0);
    llmcpp::test::check_contains(result.m_out, {"\"signature\": \"void Counter::report() const\"",
                                                "\"prompt\": \"Store the sum of values in total.\"",
                                                "Balanced braces in prompts are fine: {",
                                                "\"name\": \"doubled\""});
}

// Verify target policy, prompt resolution, transport visibility, and cache metadata.
TEST_CASE("target options and prompt files reach the agent", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "prompt.md") << "Replacement instructions.";
    std::ofstream(work.path() / "rules.md") << "Project rules.";
    std::ofstream(work.path() / "config.json")
        << R"json({"api_key":"secret-value","project":"scores"})json";
    llmcpp::test::TestCommandResult result = work.mock(
        "json/test_options.json",
        {"--llm", "-fllm-cache-dir=cache", "-fllm-model=default-model", "-fllm-max-attempts=5",
         "-fllm-timeout=10", "-fllm-system-prompt=prompt.md", "-fllm-append-system-prompt=rules.md",
         "-fllm-agent-config=config.json", "-fllm-transcript=trace.jsonl", "test_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = llmcpp::test::read_file(work.path() / "test_options.log");
    llmcpp::test::check_contains(log, {"Replacement instructions.", "Project rules.",
                                       "target-model", "default-model", "\"max_attempts\": 12",
                                       "\"timeout_seconds\": 120", "\"cache\": \"disabled\"",
                                       "\"protocol_version\": 1", "secret-value"});
    std::string transcript = llmcpp::test::read_file(work.path() / "trace.jsonl");
    CHECK(transcript.find("secret-value") == std::string::npos);
    llmcpp::test::check_contains(transcript, {"[redacted]", "get_task", "try_compile", "submit"});
    unsigned entries = 0;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        ++entries;
        std::string cache = llmcpp::test::read_file(entry.path());
        llmcpp::test::check_contains(
            cache, {"// version: llmcpp-cache-3", "// context:", "// system_prompt:", "// agent:"});
        CHECK(entry.path().stem().string().size() == 7);
    }
    CHECK(entries == 1);
    llmcpp::test::TestCommandResult replay = work.llmcpp(
        {"--llm", "-fllm-cache-dir=cache", "-fllm-regenerate", "-fllm-model=default-model",
         "-fllm-max-attempts=5", "-fllm-timeout=10", "-fllm-system-prompt=prompt.md",
         "-fllm-append-system-prompt=rules.md", "-fllm-agent-config=config.json",
         "-fllm-agent=" + (fs::path(LLMCPP_PATH).parent_path() / "llmcpp-agent").string() +
             " --replay trace.jsonl",
         "test_options.cpp"});
    INFO(replay.m_err);
    REQUIRE(replay.m_status == 0);
}

// Verify that changes to visible headers and instructions invalidate reviewed bodies.
TEST_CASE("cache tracks context and system instructions", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "cached.cpp")
        << "#include \"include/test_context.h\"\n__llm__() int cached() { Return the score. }\n";
    llmcpp::test::TestCommandResult generated =
        work.mock("json/test_options.json", {"--llm", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(generated.m_err);
    REQUIRE(generated.m_status == 0);
    llmcpp::test::TestCommandResult offline =
        work.llmcpp({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    SECTION("header contents")
    {
        std::ofstream(work.path() / "include/test_score.h") << "inline constexpr int score = 8;\n";
    }
    SECTION("system prompt")
    {
        std::ofstream(work.path() / "rules.md") << "Use a different implementation style.";
    }
    SECTION("metadata")
    {
        for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
            std::ofstream(entry.path())
                << "// llmcpp cache entry\n// model: mock\n// ---\nreturn 7;\n";
        }
    }
    std::vector<std::string> args{"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"};
    if (fs::exists(work.path() / "rules.md")) {
        args.push_back("-fllm-append-system-prompt=rules.md");
    }
    offline = work.llmcpp(args);
    CHECK(offline.m_status != 0);
    llmcpp::test::check_contains(offline.m_err, {"no cached body"});
}

// Accept bare modifiers, empty option lists, and configured modifiers together.
TEST_CASE("modifier parentheses are optional", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "modifiers.cpp")
        << "#include \"include/test_context.h\"\n"
        << "__llm__ int bare() { Return the score. }\n"
        << "__llm__() int empty() { Return the score. }\n"
        << "__llm__(max_attempts(2)) int configured() { Return the score. }\n";
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_options.json", {"--llm", "-fllm-no-cache", "modifiers.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string source = llmcpp::test::read_file(work.path() / "modifiers.llm.cpp");
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "return score;") == 3);
}

// Verify compiler identification during preprocessing, generation, and compilation.
TEST_CASE("compiler identification macro is defined", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "macro.cpp") << "#if !defined(__LLMCPP__) || __LLMCPP__ != 1\n"
                                             << "#error missing compiler identification\n"
                                             << "#endif\n"
                                             << "#include \"include/test_context.h\"\n"
                                             << "__llm__ int generated() { Return the score. }\n";
    llmcpp::test::TestCommandResult preprocess = work.llmcpp({"-E", "macro.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::TestCommandResult compile =
        work.mock("json/test_options.json", {"-fsyntax-only", "-fllm-no-cache", "macro.cpp"});
    INFO(compile.m_err);
    REQUIRE(compile.m_status == 0);
    std::ofstream(work.path() / "plain.cpp")
        << "#ifndef __LLMCPP__\n#error missing compiler identification\n#endif\n"
        << "static_assert(__LLMCPP__ == 1);\n";
    llmcpp::test::TestCommandResult plain = work.llmcpp({"-fsyntax-only", "plain.cpp"});
    INFO(plain.m_err);
    REQUIRE(plain.m_status == 0);
}

// Enforce target limits even when command-line defaults allow more work.
TEST_CASE("target generation budgets are enforced", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    SECTION("submission attempts")
    {
        std::ofstream(work.path() / "budget.cpp") << "__llm__(max_attempts(1)) int budget() {}\n";
        std::ofstream(work.path() / "json/test_budget.json") << R"json({
            "functions": {"*": [
                {"tool":"submit","arguments":{"body":"return missing;"}},
                {"tool":"submit","arguments":{"body":"return 7;"}}
            ]}
        })json";
        llmcpp::test::TestCommandResult result =
            work.mock("json/test_budget.json",
                      {"--llm", "-fllm-no-cache", "-fllm-max-attempts=4", "budget.cpp"});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "test_budget.log"),
                                     {"last allowed attempt"});
    }
    SECTION("generation timeout")
    {
        std::ofstream(work.path() / "budget.cpp") << "__llm__(timeout(1)) void budget() {}\n";
        std::ofstream(work.path() / "json/test_budget.json") << R"json({
            "functions": {"*": [{"sleep":3},{"tool":"submit","arguments":{"body":""}}]}
        })json";
        llmcpp::test::TestCommandResult result = work.mock(
            "json/test_budget.json", {"--llm", "-fllm-no-cache", "-fllm-timeout=10", "budget.cpp"});
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"agent timed out after 1s"});
    }
}

// Reject malformed policies before contacting any model.
TEST_CASE("invalid generation configuration is diagnosed", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &options :
         {"timeout(0)", "model(2)", "cache(\"v1\"), no_cache", "timeout(1), timeout(2)",
          "unknown(1)", "offline, no_cache", "offline, offline", "offline(1)", "key(\"xyz\")",
          "key(\"abcdef\")", "key(2)", "key(\"abcdef0\"), no_cache"}) {
        std::ofstream(work.path() / "invalid.cpp") << "__llm__(" << options << ") int f() {}\n";
        llmcpp::test::TestCommandResult result = work.llmcpp({"-fllm-dump-context", "invalid.cpp"});
        INFO(options);
        CHECK(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"error:"});
    }
    llmcpp::test::TestCommandResult missing =
        work.llmcpp({"-fllm-system-prompt=missing.md", "test_failure.cpp"});
    CHECK(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"cannot read system prompt"});
    std::ofstream(work.path() / "invalid.json") << "[]";
    llmcpp::test::TestCommandResult config =
        work.llmcpp({"-fllm-agent-config=invalid.json", "test_failure.cpp"});
    CHECK(config.m_status != 0);
    llmcpp::test::check_contains(config.m_err, {"agent configuration must be a JSON object"});
}

// Verify generated source, native compilation, preprocessing, and caching.
TEST_CASE("generated sources compile and cache reproducibly", "[generation][cache]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate = work.mock(
        "json/test_all_forms.json", {"--llm", "-fllm-cache-dir=cache", "test_all_forms.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    fs::path generated = work.path() / "test_all_forms.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = llmcpp::test::read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(llmcpp::test::count_occurrences(source, "// model: mock-model") == 8);
    CHECK(llmcpp::test::count_occurrences(source, "// prompt:") == 8);

    llmcpp::test::TestCommandResult build =
        work.llmcpp({"test_all_forms.llm.cpp", "-o", "from-llm-cpp"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    llmcpp::test::TestCommandResult run = work.run("./from-llm-cpp");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "test_all_forms.expected"));

    llmcpp::test::TestCommandResult gxx =
        work.run("g++", {"-std=c++17", "test_all_forms.llm.cpp", "-o", "with-gxx"});
    INFO(gxx.m_err);
    REQUIRE(gxx.m_status == 0);
    run = work.run("./with-gxx");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "test_all_forms.expected"));

    llmcpp::test::TestCommandResult offline = work.llmcpp(
        {"-fllm-offline", "-fllm-cache-dir=cache", "test_all_forms.cpp", "-o", "direct"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    run = work.run("./direct");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == llmcpp::test::read_file(work.path() / "test_all_forms.expected"));

    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(24);
    fs::last_write_time(generated, oldTime);
    llmcpp::test::TestCommandResult regenerate =
        work.llmcpp({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "test_all_forms.cpp"});
    INFO(regenerate.m_err);
    REQUIRE(regenerate.m_status == 0);
    CHECK(fs::last_write_time(generated) == oldTime);

    llmcpp::test::TestCommandResult preprocess = work.llmcpp(
        {"--llm", "-E", "-fllm-offline", "-fllm-cache-dir=cache", "test_all_forms.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "test_all_forms.llm.ii"),
                                 {"++count;"});

    llmcpp::test::TestCommandResult multiple =
        work.llmcpp({"--llm", "test_all_forms.cpp", "test_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Require cached bodies for individual targets without contacting an agent.
TEST_CASE("offline modifier requires a cached body", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__(       ) int answer() { Return 42. }\n";
    auto result =
        work.mock("json/test_return_values.json", {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::ofstream(work.path() / "answer.cpp") << "__llm__(offline) int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-cache-dir=cache",
                                  "-fllm-agent=/nonexistent/agent", "answer.cpp"};
    SECTION("cache hit") {}
    SECTION("regeneration cannot contact an agent")
    {
        args.push_back("-fllm-regenerate");
    }
    SECTION("modifier overrides disabled cache")
    {
        args.push_back("-fllm-no-cache");
    }
    SECTION("cache miss")
    {
        args.push_back("-fllm-cache-dir=missing-cache");
    }
    result = work.llmcpp(args);
    INFO(result.m_err);
    if (args.back() == "-fllm-cache-dir=missing-cache") {
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"no cached body", "offline"});
        CHECK(result.m_err.find("failed to start") == std::string::npos);
    } else {
        REQUIRE(result.m_status == 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
}

// Pin cached implementations independently of prompts and compilation context.
TEST_CASE("explicit cache keys pin generated bodies", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(key(\"ABCDEF0123\")) int answer() { Return 42. }\n";
    auto result = work.mock("json/test_return_values.json",
                            {"--llm", "-fllm-no-cache", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache = work.path() / "cache/abcdef0.cpp";
    REQUIRE(fs::exists(cache));
    std::string metadata = llmcpp::test::read_file(cache);
    llmcpp::test::check_contains(metadata, {"// key: abcdef0123\n", "return 42;"});
    std::ofstream(work.path() / "answer.cpp")
        << "__llm__(offline, key(\"abcdef0\")) int answer() { Return something else. }\n";
    SECTION("cache hit ignores changed inputs")
    {
        std::ofstream(work.path() / "rules.md") << "Different system instructions.\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "-fllm-backend=claude",
                              "-fllm-system-prompt=rules.md", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(cache) == metadata);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
    SECTION("prefix selects an ordinary computed cache hash")
    {
        std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
        result = work.mock("json/test_return_values.json",
                           {"--llm", "-fllm-cache-dir=ordinary-cache", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        std::string ordinary =
            llmcpp::test::read_file(fs::directory_iterator(work.path() / "ordinary-cache")->path());
        std::string key = ordinary.substr(ordinary.find("// key: ") + 8, 64);
        std::ofstream(work.path() / "answer.cpp") << "__llm__(offline, key(\"" << key.substr(0, 7)
                                                  << "\")) int answer() { A changed prompt. }\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=ordinary-cache", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        llmcpp::test::check_contains(llmcpp::test::read_file(work.path() / "answer.llm.cpp"),
                                     {"return 42;"});
    }
    SECTION("ambiguous prefix is rejected")
    {
        std::string other = metadata;
        other.replace(other.find("abcdef0123"), 10, "abcdef0456");
        std::ofstream(work.path() / "cache/abcdef0456.cpp") << other;
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"ambiguous cache key"});
    }
    SECTION("an incompatible pinned body still fails compilation")
    {
        std::ofstream(work.path() / "answer.cpp")
            << "__llm__(offline, key(\"abcdef0\")) void answer() {}\n";
        result = work.llmcpp({"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"does not compile"});
    }
}

// Verify short hashes, prefix collisions, and stable metadata across cache hits.
TEST_CASE("cache hashes abbreviate without losing identity", "[generation][cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
    auto result =
        work.mock("json/test_return_values.json", {"--llm", "-fllm-cache-dir=cache", "answer.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    fs::path cache;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        cache = entry.path();
    }
    REQUIRE(cache.stem().string().size() == 7);
    std::string metadata = llmcpp::test::read_file(cache);
    size_t keyBegin = metadata.find("// key: ") + 8;
    std::string key = metadata.substr(keyBegin, 64);
    std::string original = llmcpp::test::read_file(work.path() / "answer.llm.cpp");
    CHECK(original.find("// key: " + key.substr(0, 7) + "\n") != std::string::npos);
    llmcpp::test::check_contains(original, {"// context:", "// policy:", "// prompt:", "// ---"});
    CHECK(original.find("// llmcpp: generated") == std::string::npos);
    std::vector<std::string> args{"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "answer.cpp"};

    SECTION("default cache hit preserves metadata")
    {
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp") == original);
    }
    SECTION("explicit abbreviation does not invalidate the cache")
    {
        args.push_back("-fllm-hash-abbrev=10");
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp")
                  .find("// key: " + key.substr(0, 10) + "\n") != std::string::npos);
    }
    SECTION("legacy full-length filename remains readable")
    {
        fs::rename(cache, cache.parent_path() / (key + ".cpp"));
        result = work.llmcpp(args);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp") == original);
    }
    SECTION("ambiguous prefix expands source and new cache filename")
    {
        std::string other = key;
        other[7] = key[7] == 'a' ? 'b' : 'a';
        std::string conflicting = metadata;
        conflicting.replace(keyBegin, 64, other);
        std::ofstream(cache.parent_path() / (other + ".cpp")) << conflicting;
        args.erase(args.begin() + 1);
        args.push_back("-fllm-regenerate");
        result = work.mock("json/test_return_values.json", args);
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
        CHECK(llmcpp::test::read_file(cache.parent_path() / (other + ".cpp")) == conflicting);
        CHECK(llmcpp::test::read_file(work.path() / "answer.llm.cpp")
                  .find("// key: " + key.substr(0, 8) + "\n") != std::string::npos);
    }
    SECTION("new cache filenames honor explicit length")
    {
        result = work.mock("json/test_return_values.json", {"--llm", "-fllm-cache-dir=long-cache",
                                                            "-fllm-hash-abbrev=10", "answer.cpp"});
        REQUIRE(result.m_status == 0);
        REQUIRE(fs::exists(work.path() / "long-cache" / (key.substr(0, 10) + ".cpp")));
    }
    SECTION("an occupied prefix is not overwritten")
    {
        std::ofstream(cache) << "unrelated cache contents\n";
        result = work.mock("json/test_return_values.json",
                           {"--llm", "-fllm-regenerate", "-fllm-cache-dir=cache", "answer.cpp"});
        INFO(result.m_err);
        REQUIRE(result.m_status == 0);
        CHECK(llmcpp::test::read_file(cache) == "unrelated cache contents\n");
        CHECK(fs::exists(cache.parent_path() / (key.substr(0, 8) + ".cpp")));
    }
}

// Reject hash lengths outside the digest size or nondecimal values.
TEST_CASE("hash abbreviation validates its length", "[options]")
{
    llmcpp::test::TestWorkspace work;
    for (const char *value : {"0", "65", "-1", "abc", ""}) {
        auto result = work.llmcpp(
            {std::string("-fllm-hash-abbrev=") + value, "-fsyntax-only", "test_all_forms.cpp"});
        REQUIRE(result.m_status != 0);
        llmcpp::test::check_contains(result.m_err, {"invalid value for -fllm-hash-abbrev"});
    }
}

// Verify that C++ output suffixes select generated-source mode.
TEST_CASE("C++ output filenames imply source generation", "[generation]")
{
    llmcpp::test::TestWorkspace work;
    for (const std::string &output :
         {"main.llm.cpp", "anything.cpp", "anything.cc", "anything.cxx"}) {
        for (bool joined : {false, true}) {
            std::vector<std::string> args{"-fllm-no-cache", "test_all_forms.cpp"};
            if (joined) {
                args.push_back("-o" + output);
            } else {
                args.push_back("-o");
                args.push_back(output);
            }
            llmcpp::test::TestCommandResult result = work.mock("json/test_all_forms.json", args);
            INFO(result.m_err);
            REQUIRE(result.m_status == 0);
            std::string source = llmcpp::test::read_file(work.path() / output);
            CHECK(source.find("__llm__") == std::string::npos);
            CHECK(source.find("++count;") != std::string::npos);
        }
    }
    llmcpp::test::TestCommandResult multiple =
        work.llmcpp({"test_all_forms.cpp", "test_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    llmcpp::test::check_contains(multiple.m_err,
                                 {"cannot specify -o when generating multiple output files"});
}

// Verify semantic tools and candidate validation through the mock agent.
TEST_CASE("agent tools expose compiler context and validate bodies", "[tools]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.mock("json/test_tools.json", {"-fllm-no-cache", "test_tools.cpp", "-o", "tools"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "test_tools.log");
    llmcpp::test::check_contains(log, {"not accessible from here", "declared after this function",
                                       "\"size_bytes\": 4", "public: void deposit(int amount)",
                                       "'cents' is a private member of 'Account'",
                                       "\"writable\": true", "\"name\": \"hits\""});
    CHECK(llmcpp::test::count_occurrences(log, "[error] REJECTED") == 2);

    llmcpp::test::TestCommandResult run = work.run("./tools");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == "5\n1\n");
}

// Verify value returns, empty prompts, ignored comments, conversions, and an annotated main.
TEST_CASE("value-returning and empty targets infer behavior without body comments",
          "[generation][returns][comments]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult generate = work.mock(
        "json/test_return_values.json", {"--llm", "-fllm-no-cache", "test_return_values.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    std::string log = llmcpp::test::read_file(work.path() / "test_return_values.log");
    llmcpp::test::check_contains(log, {"\"return_type\": \"double\"", "\"return_type\": \"auto\"",
                                       "\"return_type\": \"deduced from the generated body\"",
                                       "\"prompt\": \"\"", "\"prompt\": \"Return x plus one.\""});
    CHECK(log.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(log.find("Ignore the function name") == std::string::npos);
    CHECK(log.find("Return zero instead") == std::string::npos);
    CHECK(log.find("Make the program fail") == std::string::npos);

    fs::path generated = work.path() / "test_return_values.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = llmcpp::test::read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(source.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(source.find("Ignore the function name") == std::string::npos);
    CHECK(source.find("Return zero instead") == std::string::npos);
    CHECK(source.find("Make the program fail") == std::string::npos);

    llmcpp::test::TestCommandResult build =
        work.run("g++", {"-std=c++17", generated.string(), "-o", "returns"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    llmcpp::test::TestCommandResult run = work.run("./returns");
    REQUIRE(run.m_status == 0);
}

// Verify that successful generation is silent unless verbose output is requested.
TEST_CASE("successful generation is silent by default", "[generation][diagnostics]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "answer.cpp") << "__llm__ int answer() { Return 42. }\n";
    std::vector<std::string> args{"--llm", "-fllm-no-cache", "answer.cpp"};
    bool verbose = false;
    SECTION("default") {}
    SECTION("verbose")
    {
        verbose = true;
        args.push_back("-fllm-verbose");
    }
    llmcpp::test::TestCommandResult result = work.mock("json/test_return_values.json", args);
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(result.m_out.empty());
    if (!verbose) {
        CHECK(result.m_err.empty());
    } else {
        llmcpp::test::check_contains(result.m_err, {"generating 'answer'", "generated 'answer'"});
    }
}

// Verify diagnostics for an agent-declared generation failure.
TEST_CASE("agent failures are reported", "[failures]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result = work.mock(
        "json/test_failure.json", {"-fllm-no-cache", "test_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(
        result.m_err, {"LLM failed to generate a body for 'f': mock gave up",
                       "last rejected attempt", "use of undeclared identifier 'not_declared'"});
}

// Verify that credentials and installed programs never select a backend.
TEST_CASE("generation requires an explicit backend", "[generation][options]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult missing =
        work.llmcpp({"-fllm-no-cache", "test_failure.cpp"}, {{"LLMCPP_BACKEND", ""},
                                                             {"LLMCPP_AGENT", ""},
                                                             {"ANTHROPIC_API_KEY", "test-key"},
                                                             {"OPENAI_API_KEY", "test-key"},
                                                             {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                                             {"LLMCPP_CLAUDE", MOCK_AGENT_PATH}});
    REQUIRE(missing.m_status != 0);
    llmcpp::test::check_contains(missing.m_err, {"select an LLM backend", "-fllm-backend"});
    for (const std::string &backend : {"auto", "unknown", ""}) {
        llmcpp::test::TestCommandResult invalid =
            work.llmcpp({"-fllm-backend=" + backend, "test_failure.cpp"});
        CHECK(invalid.m_status != 0);
    }
}

// Verify that changing backends cannot reuse a previously generated body.
TEST_CASE("cache separates selected backends", "[cache][options]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "cached.cpp") << "__llm__ int answer() { Return 42. }\n";
    llmcpp::test::TestCommandResult generated =
        work.mock("json/test_return_values.json",
                  {"--llm", "-fllm-backend=codex", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(generated.m_status == 0);
    llmcpp::test::TestCommandResult same = work.llmcpp(
        {"--llm", "-fllm-backend=codex", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(same.m_status == 0);
    llmcpp::test::TestCommandResult changed = work.llmcpp(
        {"--llm", "-fllm-backend=claude", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    REQUIRE(changed.m_status != 0);
    llmcpp::test::check_contains(changed.m_err, {"no cached body"});
}

// Verify that Anthropic generation runs in-process without the Python agent.
TEST_CASE("native Anthropic client completes a compiler tool loop", "[generation][anthropic]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeAnthropicServer server;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-no-cache", "test_failure.cpp", "-o", "native-anthropic"},
                    {{"LLMCPP_AGENT", ""},
                     {"LLMCPP_BACKEND", "anthropic"},
                     {"ANTHROPIC_API_KEY", "test-key"},
                     {"ANTHROPIC_BASE_URL", server.base_url()},
                     {"LLMCPP_MODEL", "requested-test-model"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());

    std::vector<std::string> requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"\"model\":\"requested-test-model\"", "\"get_task\"",
                                               "\"messages\"", "\"system\""});
    llmcpp::test::check_contains(
        requests[1], {"\"tool_use_id\":\"call-1\"", "\"tool_result\"", "Do something impossible."});
    llmcpp::test::check_contains(
        requests[2], {"\"tool_use_id\":\"call-2\"", "compiles without errors or warnings"});

    llmcpp::test::TestCommandResult run = work.run("./native-anthropic");
    REQUIRE(run.m_status == 0);
}

// Verify that OpenAI generation runs in-process without the Python agent.
TEST_CASE("native OpenAI client completes a compiler tool loop", "[generation][openai]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    std::vector<std::string> args{"-fllm-no-cache", "-fllm-backend=openai", "test_failure.cpp",
                                  "-o", "native-openai"};
    SECTION("native client") {}
    SECTION("bundled Python agent")
    {
        std::ofstream(work.path() / "openai.json") << "{}";
        args.push_back("-fllm-agent-config=openai.json");
    }
    llmcpp::test::TestCommandResult result =
        work.llmcpp(args, {{"LLMCPP_AGENT", ""},
                           {"LLMCPP_BACKEND", "anthropic"},
                           {"OPENAI_API_KEY", "test-key"},
                           {"OPENAI_BASE_URL", server.base_url()},
                           {"LLMCPP_MODEL", "requested-test-model"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());

    std::vector<std::string> requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"\"model\"", "\"requested-test-model\"",
                                               "\"get_task\"", "\"instructions\"", "\"function\""});
    llmcpp::test::check_contains(requests[1], {"\"previous_response_id\"", "\"response-1\"",
                                               "\"function_call_output\"", "call-1",
                                               "Do something impossible."});
    llmcpp::test::check_contains(requests[2], {"\"previous_response_id\"", "\"response-2\"",
                                               "call-2", "compiles without errors or warnings"});

    llmcpp::test::TestCommandResult run = work.run("./native-openai");
    REQUIRE(run.m_status == 0);
}

// Verify that a custom Python agent translates compiler tools for a model server.
TEST_CASE("custom chat agent completes a compiler tool loop", "[generation][agent]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestFakeOpenAIServer server;
    fs::path adapter = work.path() / "python/test_chat_agent.py";
    std::ofstream(work.path() / "chat.json")
        << "{\"base_url\":\"" << server.base_url() << "/v1\",\"model\":\"config-model\"}";
    llmcpp::test::TestCommandResult result = work.llmcpp(
        {"-fllm-no-cache", "-fllm-agent=python3 " + llmcpp::test::shell_quote(adapter.string()),
         "-fllm-agent-config=chat.json", "-fllm-model=local-model", "test_failure.cpp", "-o",
         "custom-agent"},
        {{"LOCAL_MODEL_API_KEY", "test-key"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());
    auto requests = server.requests();
    REQUIRE(requests.size() == 3);
    llmcpp::test::check_contains(requests[0], {"local-model", "system", "get_task"});
    CHECK(requests[0].find("config-model") == std::string::npos);
    llmcpp::test::check_contains(requests[1],
                                 {"Do something impossible.", "timeout_seconds", "max_attempts"});
    CHECK(work.run("./custom-agent").m_status == 0);
}

// Verify that the Codex CLI connects to the compiler through the MCP bridge.
TEST_CASE("Codex CLI completes a compiler tool loop", "[generation][codex]")
{
    llmcpp::test::TestWorkspace work;
    std::ofstream(work.path() / "codex.json")
        << "{\"effort\":\"high\",\"executable\":\"" << MOCK_AGENT_PATH << "\"}";
    std::vector<std::string> args{"-fllm-no-cache", "test_failure.cpp", "-o", "codex"};
    std::string backend = "codex";
    SECTION("environment defaults") {}
    SECTION("command line overrides environment")
    {
        args.push_back("-fllm-backend=codex");
        backend = "claude";
    }
    SECTION("agent configuration")
    {
        args.push_back("-fllm-agent-config=codex.json");
    }
    llmcpp::test::TestCommandResult result = work.llmcpp(args, {{"ANTHROPIC_API_KEY", ""},
                                                                {"LLMCPP_AGENT", ""},
                                                                {"LLMCPP_BACKEND", backend},
                                                                {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                                                {"LLMCPP_EFFORT", "high"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    llmcpp::test::TestCommandResult run = work.run("./codex");
    REQUIRE(run.m_status == 0);
}

// Verify diagnostics when the configured agent cannot start.
TEST_CASE("a missing agent is reported", "[failures]")
{
    llmcpp::test::TestWorkspace work;
    llmcpp::test::TestCommandResult result =
        work.llmcpp({"-fllm-agent=/nonexistent/llmcpp-agent", "-fllm-no-cache", "test_failure.cpp",
                     "-o", "failure"});
    REQUIRE(result.m_status != 0);
    llmcpp::test::check_contains(result.m_err, {"failed to start"});
}
