/*
 * C++ file for Catch2 integration coverage for llmc++.
 */

// Catch2 header for integration test declarations and assertions.
#include <catch2/catch_test_macros.hpp>

// Header-only HTTP server for exercising the built-in API clients.
#include <httplib.h>

// Standard and POSIX headers for isolated filesystem command tests.
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Namespace for integration-test support private to this translation unit.
namespace
{

    // Quote one value for a POSIX shell command.
    std::string shell_quote(const std::string &value)
    {
        std::string quoted = "'";
        for (char c : value) {
            quoted += c == '\'' ? "'\\''" : std::string(1, c);
        }
        return quoted + "'";
    }

    // Read an entire test file.
    std::string read_file(const fs::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    // Count non-overlapping occurrences of a substring.
    size_t count_occurrences(const std::string &text, const std::string &needle)
    {
        size_t count = 0;
        for (size_t pos = 0; (pos = text.find(needle, pos)) != std::string::npos;
             pos += needle.size()) {
            ++count;
        }
        return count;
    }

    // Check that text contains every expected fragment.
    void check_contains(const std::string &text, std::initializer_list<const char *> needles)
    {
        INFO(text);
        for (const char *needle : needles) {
            INFO("Expected output to contain: " << needle);
            CHECK(text.find(needle) != std::string::npos);
        }
    }

    // Structure for one child command's status and captured streams.
    struct CommandResult
    {
        int m_status;
        std::string m_out;
        std::string m_err;
    };

    // Class for managing isolated integration fixtures and command outputs.
    class Workspace
    {
    public:
        // Create and populate an isolated test workspace.
        Workspace()
        {
            static std::atomic<unsigned> next{0};
            m_root = fs::temp_directory_path() /
                     ("llmcpp-catch2-" + std::to_string(getpid()) + "-" + std::to_string(next++));
            fs::create_directories(m_root);
            for (const fs::directory_entry &entry : fs::directory_iterator(TEST_CASES_DIR)) {
                fs::copy(entry.path(), m_root / entry.path().filename(),
                         fs::copy_options::overwrite_existing | fs::copy_options::recursive);
            }
        }

        // Remove the isolated test workspace.
        ~Workspace()
        {
            std::error_code error;
            fs::remove_all(m_root, error);
        }

        // Get the test workspace path.
        const fs::path &path() const
        {
            return m_root;
        }

        // Run a command inside the test workspace.
        CommandResult run(const std::string &executable,
                          const std::vector<std::string> &arguments = {},
                          const std::vector<std::pair<std::string, std::string>> &environment = {})
        {
            fs::path outPath = m_root / ("command-" + std::to_string(m_command++) + ".out");
            fs::path errPath = m_root / ("command-" + std::to_string(m_command) + ".err");
            std::ostringstream shell;
            shell << "cd " << shell_quote(m_root.string()) << " && ";
            for (const auto &[Name, Value] : environment) {
                shell << Name << '=' << shell_quote(Value) << ' ';
            }
            shell << shell_quote(executable);
            for (const std::string &argument : arguments) {
                shell << ' ' << shell_quote(argument);
            }
            shell << " >" << shell_quote(outPath.string()) << " 2>"
                  << shell_quote(errPath.string());

            int raw = std::system(shell.str().c_str());
            int status = raw == -1 ? -1 : WIFEXITED(raw) ? WEXITSTATUS(raw) : 128;
            return {status, read_file(outPath), read_file(errPath)};
        }

        // Run llmc++ with test arguments.
        CommandResult
        llmcxx(const std::vector<std::string> &arguments,
               const std::vector<std::pair<std::string, std::string>> &environment = {})
        {
            return run(LLMCPP_PATH, arguments, environment);
        }

        // Run llmc++ against a scripted mock agent.
        CommandResult mock(const std::string &script, std::vector<std::string> arguments)
        {
            fs::path log = m_root / (fs::path(script).stem().string() + ".log");
            arguments.insert(arguments.begin(), "-fllm-agent=" + std::string(MOCK_AGENT_PATH));
            arguments.insert(arguments.begin() + 1, "-fllm-quiet");
            return run(LLMCPP_PATH, arguments,
                       {{"LLMCPP_MOCK_SCRIPT", (m_root / script).string()},
                        {"LLMCPP_MOCK_LOG", log.string()}});
        }

    private:
        fs::path m_root;
        unsigned m_command = 0;
    };

    // Local Messages API endpoint that drives get_task, try_compile, and submit.
    class FakeAnthropicServer
    {
    public:
        FakeAnthropicServer()
        {
            m_server.Post("/v1/messages", [this](const httplib::Request &request,
                                                 httplib::Response &response) {
                unsigned call = ++m_calls;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_requests.push_back(request.body);
                    if (request.get_header_value("x-api-key") != "test-key") {
                        m_problem = "missing or incorrect x-api-key";
                    }
                    if (request.get_header_value("anthropic-version") != "2023-06-01") {
                        m_problem = "missing or incorrect anthropic-version";
                    }
                }

                const char *content = nullptr;
                if (call == 1) {
                    content =
                        R"json({"type":"tool_use","id":"call-1","name":"get_task","input":{}})json";
                } else if (call == 2) {
                    content =
                        R"json({"type":"tool_use","id":"call-2","name":"try_compile","input":{"body":""}})json";
                } else if (call == 3) {
                    content =
                        R"json({"type":"tool_use","id":"call-3","name":"submit","input":{"body":""}})json";
                } else {
                    response.status = 500;
                    response.set_content("unexpected extra request", "text/plain");
                    return;
                }
                response.set_content(
                    std::string(R"json({"model":"native-test-model","content":[)json") + content +
                        "]}",
                    "application/json");
            });
            m_port = m_server.bind_to_any_port("127.0.0.1");
            if (m_port <= 0) {
                throw std::runtime_error("could not bind fake Anthropic server");
            }
            m_thread = std::thread([this] {
                m_server.listen_after_bind();
            });
        }

        ~FakeAnthropicServer()
        {
            m_server.stop();
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        std::string base_url() const
        {
            return "http://127.0.0.1:" + std::to_string(m_port);
        }

        unsigned calls() const
        {
            return m_calls;
        }

        std::vector<std::string> requests() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_requests;
        }

        std::string problem() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_problem;
        }

    private:
        httplib::Server m_server;
        int m_port = -1;
        std::thread m_thread;
        std::atomic<unsigned> m_calls{0};
        mutable std::mutex m_mutex;
        std::vector<std::string> m_requests;
        std::string m_problem;
    };

    // Local Responses API endpoint that drives get_task, try_compile, and submit.
    class FakeOpenAIServer
    {
    public:
        FakeOpenAIServer()
        {
            m_server.Post("/v1/chat/completions", [this](const httplib::Request &request,
                                                         httplib::Response &response) {
                unsigned call = ++m_calls;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_requests.push_back(request.body);
                    if (request.get_header_value("Authorization") != "Bearer test-key") {
                        m_problem = "missing or incorrect Authorization header";
                    }
                }
                const char *name = call == 1 ? "get_task" : call == 2 ? "try_compile" : "submit";
                const char *arguments = call == 1 ? "{}" : R"json({\"body\":\"\"})json";
                response.set_content(
                    std::string(
                        R"json({"model":"local-model","choices":[{"message":{"role":"assistant","tool_calls":[{"id":"call-)json") +
                        std::to_string(call) +
                        R"json(","type":"function","function":{"name":")json" + name +
                        R"json(","arguments":")json" + arguments + R"json("}}]}}]})json",
                    "application/json");
            });
            m_server.Post("/v1/responses", [this](const httplib::Request &request,
                                                  httplib::Response &response) {
                unsigned call = ++m_calls;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_requests.push_back(request.body);
                    if (request.get_header_value("Authorization") != "Bearer test-key") {
                        m_problem = "missing or incorrect Authorization header";
                    }
                }

                const char *name = nullptr;
                const char *arguments = nullptr;
                if (call == 1) {
                    name = "get_task";
                    arguments = "{}";
                } else if (call == 2) {
                    name = "try_compile";
                    arguments = R"json({\"body\":\"\"})json";
                } else if (call == 3) {
                    name = "submit";
                    arguments = R"json({\"body\":\"\"})json";
                } else {
                    response.status = 500;
                    response.set_content("unexpected extra request", "text/plain");
                    return;
                }
                response.set_content(
                    std::string(R"json({"id":"response-)json") + std::to_string(call) +
                        R"json(","model":"native-openai-test-model","output":[{"type":"function_call","call_id":"call-)json" +
                        std::to_string(call) + R"json(","name":")json" + name +
                        R"json(","arguments":)json" + std::string("\"") + arguments + "\"}]}",
                    "application/json");
            });
            m_port = m_server.bind_to_any_port("127.0.0.1");
            if (m_port <= 0) {
                throw std::runtime_error("could not bind fake OpenAI server");
            }
            m_thread = std::thread([this] {
                m_server.listen_after_bind();
            });
        }

        ~FakeOpenAIServer()
        {
            m_server.stop();
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        std::string base_url() const
        {
            return "http://127.0.0.1:" + std::to_string(m_port);
        }

        unsigned calls() const
        {
            return m_calls;
        }

        std::vector<std::string> requests() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_requests;
        }

        std::string problem() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_problem;
        }

    private:
        httplib::Server m_server;
        int m_port = -1;
        std::thread m_thread;
        std::atomic<unsigned> m_calls{0};
        mutable std::mutex m_mutex;
        std::vector<std::string> m_requests;
        std::string m_problem;
    };

}

// Verify diagnostics for unsupported or malformed annotations.
TEST_CASE("invalid annotations produce llmc++ diagnostics", "[diagnostics]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fsyntax-only", "case_errors.cpp"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err,
                   {"preprocessor directives are not allowed in an __llm__ function body",
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
    Workspace work;
    for (const char *mode : {"-fsyntax-only", "--llm"}) {
        DYNAMIC_SECTION(mode)
        {
            CommandResult result = work.llmcxx({mode, "case_header.cpp"});
            REQUIRE(result.m_status != 0);
            check_contains(result.m_err, {"__llm__ function in included header 'case_header.h'"});
        }
    }
}

// Verify that offline mode fails when no cached body exists.
TEST_CASE("offline mode requires cached bodies", "[cache]")
{
    Workspace work;
    CommandResult result = work.llmcxx(
        {"-fsyntax-only", "-fllm-offline", "-fllm-cache-dir=empty", "case_all_forms.cpp"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err, {"no cached body for __llm__ function 'sum' (-fllm-offline)"});
}

// Verify that plain prompt text reaches compiler-context output.
TEST_CASE("compiler context includes plain prompts", "[context]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fllm-dump-context", "case_all_forms.cpp"});
    REQUIRE(result.m_status == 0);
    check_contains(result.m_out,
                   {"\"signature\": \"void Counter::report() const\"",
                    "\"prompt\": \"Store the sum of values in total.\"",
                    "Balanced braces in prompts are fine: {", "\"name\": \"doubled\""});
}

// Verify target policy, prompt resolution, transport visibility, and cache metadata.
TEST_CASE("target options and prompt files reach the agent", "[generation][options]")
{
    Workspace work;
    std::ofstream(work.path() / "prompt.md") << "Replacement instructions.";
    std::ofstream(work.path() / "rules.md") << "Project rules.";
    std::ofstream(work.path() / "config.json")
        << R"json({"api_key":"secret-value","project":"scores"})json";
    CommandResult result = work.mock(
        "json/case_options.json",
        {"--llm", "-fllm-cache-dir=cache", "-fllm-model=default-model", "-fllm-max-attempts=5",
         "-fllm-timeout=10", "-fllm-system-prompt=prompt.md", "-fllm-append-system-prompt=rules.md",
         "-fllm-agent-config=config.json", "-fllm-transcript=trace.jsonl", "case_options.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string log = read_file(work.path() / "case_options.log");
    check_contains(log, {"Replacement instructions.", "Project rules.", "target-model",
                         "default-model", "\"max_attempts\": 12", "\"timeout_seconds\": 120",
                         "\"cache\": \"disabled\"", "\"protocol_version\": 1", "secret-value"});
    std::string transcript = read_file(work.path() / "trace.jsonl");
    CHECK(transcript.find("secret-value") == std::string::npos);
    check_contains(transcript, {"[redacted]", "get_task", "try_compile", "submit"});
    unsigned entries = 0;
    for (const auto &entry : fs::directory_iterator(work.path() / "cache")) {
        ++entries;
        std::string cache = read_file(entry.path());
        check_contains(
            cache, {"// version: llmcpp-cache-3", "// context:", "// system_prompt:", "// agent:"});
        CHECK(entry.path().stem().string().size() == 64);
    }
    CHECK(entries == 1);
    CommandResult replay = work.llmcxx(
        {"--llm", "-fllm-cache-dir=cache", "-fllm-regenerate", "-fllm-model=default-model",
         "-fllm-max-attempts=5", "-fllm-timeout=10", "-fllm-system-prompt=prompt.md",
         "-fllm-append-system-prompt=rules.md", "-fllm-agent-config=config.json",
         "-fllm-agent=" + (fs::path(LLMCPP_PATH).parent_path() / "llmcpp-agent").string() +
             " --replay trace.jsonl",
         "case_options.cpp"});
    INFO(replay.m_err);
    REQUIRE(replay.m_status == 0);
}

// Verify that changes to visible headers and instructions invalidate reviewed bodies.
TEST_CASE("cache tracks context and system instructions", "[cache][options]")
{
    Workspace work;
    std::ofstream(work.path() / "cached.cpp")
        << "#include \"include/case_context.h\"\n__llm__() int cached() { Return the score. }\n";
    CommandResult generated =
        work.mock("json/case_options.json", {"--llm", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(generated.m_err);
    REQUIRE(generated.m_status == 0);
    CommandResult offline =
        work.llmcxx({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "cached.cpp"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    SECTION("header contents")
    {
        std::ofstream(work.path() / "include/case_score.h") << "inline constexpr int score = 8;\n";
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
    offline = work.llmcxx(args);
    CHECK(offline.m_status != 0);
    check_contains(offline.m_err, {"no cached body"});
}

// Accept bare modifiers, empty option lists, and configured modifiers together.
TEST_CASE("modifier parentheses are optional", "[generation][options]")
{
    Workspace work;
    std::ofstream(work.path() / "modifiers.cpp")
        << "#include \"include/case_context.h\"\n"
        << "__llm__ int bare() { Return the score. }\n"
        << "__llm__() int empty() { Return the score. }\n"
        << "__llm__(max_attempts(2)) int configured() { Return the score. }\n";
    CommandResult result =
        work.mock("json/case_options.json", {"--llm", "-fno-llm-cache", "modifiers.cpp"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    std::string source = read_file(work.path() / "modifiers.llm.cpp");
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(count_occurrences(source, "return score;") == 3);
}

// Enforce target limits even when command-line defaults allow more work.
TEST_CASE("target generation budgets are enforced", "[generation][options]")
{
    Workspace work;
    SECTION("submission attempts")
    {
        std::ofstream(work.path() / "budget.cpp") << "__llm__(max_attempts(1)) int budget() {}\n";
        std::ofstream(work.path() / "json/case_budget.json") << R"json({
            "functions": {"*": [
                {"tool":"submit","arguments":{"body":"return missing;"}},
                {"tool":"submit","arguments":{"body":"return 7;"}}
            ]}
        })json";
        CommandResult result =
            work.mock("json/case_budget.json",
                      {"--llm", "-fno-llm-cache", "-fllm-max-attempts=4", "budget.cpp"});
        CHECK(result.m_status != 0);
        check_contains(read_file(work.path() / "case_budget.log"), {"last allowed attempt"});
    }
    SECTION("generation timeout")
    {
        std::ofstream(work.path() / "budget.cpp") << "__llm__(timeout(1)) void budget() {}\n";
        std::ofstream(work.path() / "json/case_budget.json") << R"json({
            "functions": {"*": [{"sleep":3},{"tool":"submit","arguments":{"body":""}}]}
        })json";
        CommandResult result = work.mock(
            "json/case_budget.json", {"--llm", "-fno-llm-cache", "-fllm-timeout=10", "budget.cpp"});
        CHECK(result.m_status != 0);
        check_contains(result.m_err, {"agent timed out after 1s"});
    }
}

// Reject malformed policies before contacting any model.
TEST_CASE("invalid generation configuration is diagnosed", "[options]")
{
    Workspace work;
    for (const std::string &options : {"timeout(0)", "model(2)", "cache(\"v1\"), no_cache",
                                       "timeout(1), timeout(2)", "unknown(1)"}) {
        std::ofstream(work.path() / "invalid.cpp") << "__llm__(" << options << ") int f() {}\n";
        CommandResult result = work.llmcxx({"-fllm-dump-context", "invalid.cpp"});
        INFO(options);
        CHECK(result.m_status != 0);
        check_contains(result.m_err, {"error:"});
    }
    CommandResult missing = work.llmcxx({"-fllm-system-prompt=missing.md", "case_failure.cpp"});
    CHECK(missing.m_status != 0);
    check_contains(missing.m_err, {"cannot read system prompt"});
    std::ofstream(work.path() / "invalid.json") << "[]";
    CommandResult config = work.llmcxx({"-fllm-agent-config=invalid.json", "case_failure.cpp"});
    CHECK(config.m_status != 0);
    check_contains(config.m_err, {"agent configuration must be a JSON object"});
}

// Verify generated source, native compilation, preprocessing, and caching.
TEST_CASE("generated sources compile and cache reproducibly", "[generation][cache]")
{
    Workspace work;
    CommandResult generate = work.mock("json/case_all_forms.json",
                                       {"--llm", "-fllm-cache-dir=cache", "case_all_forms.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    fs::path generated = work.path() / "case_all_forms.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(count_occurrences(source, "// llmcpp: generated (model=mock-model, key=") == 8);

    CommandResult build = work.llmcxx({"case_all_forms.llm.cpp", "-o", "from-llm-cpp"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    CommandResult run = work.run("./from-llm-cpp");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "case_all_forms.expected"));

    CommandResult gxx = work.run("g++", {"-std=c++17", "case_all_forms.llm.cpp", "-o", "with-gxx"});
    INFO(gxx.m_err);
    REQUIRE(gxx.m_status == 0);
    run = work.run("./with-gxx");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "case_all_forms.expected"));

    CommandResult offline = work.llmcxx(
        {"-fllm-offline", "-fllm-cache-dir=cache", "case_all_forms.cpp", "-o", "direct"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    run = work.run("./direct");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "case_all_forms.expected"));

    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(24);
    fs::last_write_time(generated, oldTime);
    CommandResult regenerate =
        work.llmcxx({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "case_all_forms.cpp"});
    INFO(regenerate.m_err);
    REQUIRE(regenerate.m_status == 0);
    CHECK(fs::last_write_time(generated) == oldTime);

    CommandResult preprocess = work.llmcxx(
        {"--llm", "-E", "-fllm-offline", "-fllm-cache-dir=cache", "case_all_forms.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    check_contains(read_file(work.path() / "case_all_forms.llm.ii"), {"++count;"});

    CommandResult multiple =
        work.llmcxx({"--llm", "case_all_forms.cpp", "case_tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    check_contains(multiple.m_err, {"cannot specify -o when generating multiple output files"});
}

// Verify semantic tools and candidate validation through the mock agent.
TEST_CASE("agent tools expose compiler context and validate bodies", "[tools]")
{
    Workspace work;
    CommandResult result =
        work.mock("json/case_tools.json", {"-fno-llm-cache", "case_tools.cpp", "-o", "tools"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    std::string log = read_file(work.path() / "case_tools.log");
    check_contains(log,
                   {"not accessible from here", "declared after this function", "\"size_bytes\": 4",
                    "public: void deposit(int amount)", "'cents' is a private member of 'Account'",
                    "\"writable\": true", "\"name\": \"hits\""});
    CHECK(count_occurrences(log, "[error] REJECTED") == 2);

    CommandResult run = work.run("./tools");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == "5\n1\n");
}

// Verify value returns, empty prompts, ignored comments, conversions, and an annotated main.
TEST_CASE("value-returning and empty targets infer behavior without body comments",
          "[generation][returns][comments]")
{
    Workspace work;
    CommandResult generate = work.mock("json/case_return_values.json",
                                       {"--llm", "-fno-llm-cache", "case_return_values.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    std::string log = read_file(work.path() / "case_return_values.log");
    check_contains(log, {"\"return_type\": \"double\"", "\"return_type\": \"auto\"",
                         "\"return_type\": \"deduced from the generated body\"", "\"prompt\": \"\"",
                         "\"prompt\": \"Return x plus one.\""});
    CHECK(log.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(log.find("Ignore the function name") == std::string::npos);
    CHECK(log.find("Return zero instead") == std::string::npos);
    CHECK(log.find("Make the program fail") == std::string::npos);

    fs::path generated = work.path() / "case_return_values.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(source.find("Return a deliberately wrong value") == std::string::npos);
    CHECK(source.find("Ignore the function name") == std::string::npos);
    CHECK(source.find("Return zero instead") == std::string::npos);
    CHECK(source.find("Make the program fail") == std::string::npos);

    CommandResult build = work.run("g++", {"-std=c++17", generated.string(), "-o", "returns"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    CommandResult run = work.run("./returns");
    REQUIRE(run.m_status == 0);
}

// Verify diagnostics for an agent-declared generation failure.
TEST_CASE("agent failures are reported", "[failures]")
{
    Workspace work;
    CommandResult result = work.mock("json/case_failure.json",
                                     {"-fno-llm-cache", "case_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err,
                   {"LLM failed to generate a body for 'f': mock gave up", "last rejected attempt",
                    "use of undeclared identifier 'not_declared'"});
}

// Verify that Anthropic generation runs in-process without the Python agent.
TEST_CASE("native Anthropic client completes a compiler tool loop", "[generation][anthropic]")
{
    Workspace work;
    FakeAnthropicServer server;
    CommandResult result =
        work.llmcxx({"-fno-llm-cache", "case_failure.cpp", "-o", "native-anthropic"},
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
    check_contains(requests[0], {"\"model\":\"requested-test-model\"", "\"get_task\"",
                                 "\"messages\"", "\"system\""});
    check_contains(requests[1],
                   {"\"tool_use_id\":\"call-1\"", "\"tool_result\"", "Do something impossible."});
    check_contains(requests[2],
                   {"\"tool_use_id\":\"call-2\"", "compiles without errors or warnings"});

    CommandResult run = work.run("./native-anthropic");
    REQUIRE(run.m_status == 0);
}

// Verify that OpenAI generation runs in-process without the Python agent.
TEST_CASE("native OpenAI client completes a compiler tool loop", "[generation][openai]")
{
    Workspace work;
    FakeOpenAIServer server;
    CommandResult result =
        work.llmcxx({"-fno-llm-cache", "case_failure.cpp", "-o", "native-openai"},
                    {{"LLMCPP_AGENT", ""},
                     {"LLMCPP_BACKEND", "openai"},
                     {"OPENAI_API_KEY", "test-key"},
                     {"OPENAI_BASE_URL", server.base_url()},
                     {"LLMCPP_MODEL", "requested-test-model"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());

    std::vector<std::string> requests = server.requests();
    REQUIRE(requests.size() == 3);
    check_contains(requests[0], {"\"model\":\"requested-test-model\"", "\"get_task\"",
                                 "\"instructions\"", "\"type\":\"function\""});
    check_contains(requests[1],
                   {"\"previous_response_id\":\"response-1\"", "\"type\":\"function_call_output\"",
                    "call-1", "Do something impossible."});
    check_contains(requests[2], {"\"previous_response_id\":\"response-2\"", "call-2",
                                 "compiles without errors or warnings"});

    CommandResult run = work.run("./native-openai");
    REQUIRE(run.m_status == 0);
}

// Verify that a custom Python agent translates compiler tools for a model server.
TEST_CASE("custom chat agent completes a compiler tool loop", "[generation][agent]")
{
    Workspace work;
    FakeOpenAIServer server;
    fs::path adapter = work.path() / "python/case_chat_agent.py";
    std::ofstream(work.path() / "chat.json")
        << "{\"base_url\":\"" << server.base_url() << "/v1\",\"model\":\"config-model\"}";
    CommandResult result =
        work.llmcxx({"-fno-llm-cache", "-fllm-agent=python3 " + shell_quote(adapter.string()),
                     "-fllm-agent-config=chat.json", "-fllm-model=local-model", "case_failure.cpp",
                     "-o", "custom-agent"},
                    {{"LOCAL_MODEL_API_KEY", "test-key"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);
    CHECK(server.calls() == 3);
    CHECK(server.problem().empty());
    auto requests = server.requests();
    REQUIRE(requests.size() == 3);
    check_contains(requests[0], {"local-model", "system", "get_task"});
    CHECK(requests[0].find("config-model") == std::string::npos);
    check_contains(requests[1], {"Do something impossible.", "timeout_seconds", "max_attempts"});
    CHECK(work.run("./custom-agent").m_status == 0);
}

// Verify that the Codex CLI connects to the compiler through the MCP bridge.
TEST_CASE("Codex CLI completes a compiler tool loop", "[generation][codex]")
{
    Workspace work;
    std::ofstream(work.path() / "codex.json")
        << "{\"backend\":\"codex\",\"effort\":\"high\",\"executable\":\"" << MOCK_AGENT_PATH
        << "\"}";
    std::vector<std::string> args{"-fno-llm-cache", "case_failure.cpp", "-o", "codex"};
    SECTION("environment defaults") {}
    SECTION("agent configuration")
    {
        args.push_back("-fllm-agent-config=codex.json");
    }
    CommandResult result = work.llmcxx(args, {{"ANTHROPIC_API_KEY", ""},
                                              {"LLMCPP_AGENT", ""},
                                              {"LLMCPP_BACKEND", "codex"},
                                              {"LLMCPP_CODEX", MOCK_AGENT_PATH},
                                              {"LLMCPP_EFFORT", "high"}});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    CommandResult run = work.run("./codex");
    REQUIRE(run.m_status == 0);
}

// Verify diagnostics when the configured agent cannot start.
TEST_CASE("a missing agent is reported", "[failures]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fllm-agent=/nonexistent/llmcpp-agent", "-fno-llm-cache",
                                        "case_failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err, {"failed to start"});
}
