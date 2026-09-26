/*
 * C++ file for Catch2 integration coverage for llmc++.
 */

// Catch2 header for integration test declarations and assertions.
#include <catch2/catch_test_macros.hpp>

// Standard and POSIX headers for isolated filesystem command tests.
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

// Namespace alias for test filesystem operations.
namespace fs = std::filesystem;

// Namespace for integration-test support private to this translation unit.
namespace {

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
    struct CommandResult {
        int m_status;
        std::string m_out;
        std::string m_err;
    };

    // Class for managing isolated integration fixtures and command outputs.
    class Workspace {
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
        CommandResult llmcxx(const std::vector<std::string> &arguments)
        {
            return run(LLMCXX_PATH, arguments);
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

}

// Verify diagnostics for unsupported or malformed annotations.
TEST_CASE("invalid annotations produce llmc++ diagnostics", "[diagnostics]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fsyntax-only", "errors.cpp"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err,
                   {"__llm__ function 'non_void' must return void",
                    "__llm__ function 'deduced' must declare its return type as void, not 'auto'",
                    "__llm__ function 'no_prompt' has no prompt",
                    "preprocessor directives are not allowed in an __llm__ function body",
                    "__llm__ function 'declaration_only' must have a body containing the prompt",
                    "__llm__ function 'compile_time' cannot be constexpr",
                    "__llm__ function 'try_block' cannot have a function-try-block",
                    "__llm__ function 'S::S' cannot be defaulted or deleted",
                    "a conversion operator cannot be __llm__",
                    "__llm__ cannot be used inside a macro expansion",
                    "__llm__ must be followed by a function definition or a lambda",
                    "an __llm__ lambda with a trailing return type must return void"});
}

// Verify that annotations in included headers are rejected.
TEST_CASE("annotations in headers are rejected", "[diagnostics]")
{
    Workspace work;
    for (const char *mode : {"-fsyntax-only", "--llm"}) {
        DYNAMIC_SECTION(mode)
        {
            CommandResult result = work.llmcxx({mode, "header.cpp"});
            REQUIRE(result.m_status != 0);
            check_contains(result.m_err, {"__llm__ function in included header 'header.h'"});
        }
    }
}

// Verify that offline mode fails when no cached body exists.
TEST_CASE("offline mode requires cached bodies", "[cache]")
{
    Workspace work;
    CommandResult result =
        work.llmcxx({"-fsyntax-only", "-fllm-offline", "-fllm-cache-dir=empty", "all-forms.cpp"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err, {"no cached body for __llm__ function 'sum' (-fllm-offline)"});
}

// Verify that plain prompt text reaches compiler-context output.
TEST_CASE("compiler context includes plain prompts", "[context]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fllm-dump-context", "all-forms.cpp"});
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
    CommandResult generate =
        work.mock("all-forms.json", {"--llm", "-fllm-cache-dir=cache", "all-forms.cpp"});
    INFO(generate.m_err);
    REQUIRE(generate.m_status == 0);

    fs::path generated = work.path() / "all-forms.llm.cpp";
    REQUIRE(fs::exists(generated));
    std::string source = read_file(generated);
    CHECK(source.find("__llm__") == std::string::npos);
    CHECK(count_occurrences(source, "// llmcpp: generated (model=mock-model, key=") == 8);

    CommandResult build = work.llmcxx({"all-forms.llm.cpp", "-o", "from-llm-cpp"});
    INFO(build.m_err);
    REQUIRE(build.m_status == 0);
    CommandResult run = work.run("./from-llm-cpp");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "all-forms.expected"));

    CommandResult gxx = work.run("g++", {"-std=c++17", "all-forms.llm.cpp", "-o", "with-gxx"});
    INFO(gxx.m_err);
    REQUIRE(gxx.m_status == 0);
    run = work.run("./with-gxx");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "all-forms.expected"));

    CommandResult offline =
        work.llmcxx({"-fllm-offline", "-fllm-cache-dir=cache", "all-forms.cpp", "-o", "direct"});
    INFO(offline.m_err);
    REQUIRE(offline.m_status == 0);
    run = work.run("./direct");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == read_file(work.path() / "all-forms.expected"));

    auto oldTime = fs::file_time_type::clock::now() - std::chrono::hours(24);
    fs::last_write_time(generated, oldTime);
    CommandResult regenerate =
        work.llmcxx({"--llm", "-fllm-offline", "-fllm-cache-dir=cache", "all-forms.cpp"});
    INFO(regenerate.m_err);
    REQUIRE(regenerate.m_status == 0);
    CHECK(fs::last_write_time(generated) == oldTime);

    CommandResult preprocess =
        work.llmcxx({"--llm", "-E", "-fllm-offline", "-fllm-cache-dir=cache", "all-forms.cpp"});
    INFO(preprocess.m_err);
    REQUIRE(preprocess.m_status == 0);
    check_contains(read_file(work.path() / "all-forms.llm.ii"), {"++count;"});

    CommandResult multiple = work.llmcxx({"--llm", "all-forms.cpp", "tools.cpp", "-o", "both.cpp"});
    REQUIRE(multiple.m_status != 0);
    check_contains(multiple.m_err, {"cannot specify -o when generating multiple output files"});
}

// Verify semantic tools and candidate validation through the mock agent.
TEST_CASE("agent tools expose compiler context and validate bodies", "[tools]")
{
    Workspace work;
    CommandResult result = work.mock("tools.json", {"-fno-llm-cache", "tools.cpp", "-o", "tools"});
    INFO(result.m_err);
    REQUIRE(result.m_status == 0);

    std::string log = read_file(work.path() / "tools.log");
    check_contains(log,
                   {"not accessible from here", "declared after this function", "\"size_bytes\": 4",
                    "public: void deposit(int amount)", "'cents' is a private member of 'Account'",
                    "\"writable\": true", "\"name\": \"hits\""});
    CHECK(count_occurrences(log, "[error] REJECTED") == 2);

    CommandResult run = work.run("./tools");
    REQUIRE(run.m_status == 0);
    CHECK(run.m_out == "5\n1\n");
}

// Verify diagnostics for an agent-declared generation failure.
TEST_CASE("agent failures are reported", "[failures]")
{
    Workspace work;
    CommandResult result =
        work.mock("failure.json", {"-fno-llm-cache", "failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err,
                   {"LLM failed to generate a body for 'f': mock gave up", "last rejected attempt",
                    "use of undeclared identifier 'not_declared'"});
}

// Verify diagnostics when the configured agent cannot start.
TEST_CASE("a missing agent is reported", "[failures]")
{
    Workspace work;
    CommandResult result = work.llmcxx({"-fllm-agent=/nonexistent/llmcpp-agent", "-fno-llm-cache",
                                        "failure.cpp", "-o", "failure"});
    REQUIRE(result.m_status != 0);
    check_contains(result.m_err, {"failed to start"});
}
