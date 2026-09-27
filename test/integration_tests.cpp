/*
 * C++ file for Catch2 integration coverage for llmc++.
 */

// Catch2 header for integration test declarations and assertions.
#include <catch2/catch_test_macros.hpp>

// Header-only HTTP server for exercising the built-in Anthropic client.
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
            return run(LLMCXX_PATH, arguments, environment);
        }

        // Run llmc++ against a scripted mock agent.
        CommandResult mock(const std::string &script, std::vector<std::string> arguments)
        {
            fs::path log = m_root / (fs::path(script).stem().string() + ".log");
            arguments.insert(arguments.begin(), "-fllm-agent=" + std::string(MOCK_AGENT_PATH));
            arguments.insert(arguments.begin() + 1, "-fllm-quiet");
            return run(LLMCXX_PATH, arguments,
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

// Verify that the Codex CLI connects to the compiler through the MCP bridge.
TEST_CASE("Codex CLI completes a compiler tool loop", "[generation][codex]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fno-llm-cache", "case_failure.cpp", "-o", "codex"},
                                       {{"ANTHROPIC_API_KEY", ""},
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
