/*
 * C++ file for workspace test support.
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

// Headers for integration-test support and its dependencies.
#include "llmcpp/test/test_workspace.h"
#include "llmcpp/test/test_text.h"

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
// Namespace for llmcpp integration-test support.
namespace llmcpp::test
{
    // Initialize the isolated workspace.
    TestWorkspace::TestWorkspace()
    {
        static std::atomic<unsigned> next{0};
        m_root = std::filesystem::temp_directory_path() /
                 ("llmcpp-catch2-" + std::to_string(getpid()) + "-" + std::to_string(next++));
        std::filesystem::create_directories(m_root);
        for (const std::filesystem::directory_entry &entry :
             std::filesystem::directory_iterator(TEST_FIXTURES_DIR)) {
            std::filesystem::copy(entry.path(), m_root / entry.path().filename(),
                                  std::filesystem::copy_options::overwrite_existing |
                                      std::filesystem::copy_options::recursive);
        }
    }

    // Stop and clean up the isolated workspace.
    TestWorkspace::~TestWorkspace()
    {
        std::error_code error;
        std::filesystem::remove_all(m_root, error);
    }

    // Get the workspace directory.
    const std::filesystem::path &TestWorkspace::path() const
    {
        return m_root;
    }

    // Write a source fixture into a workspace test file.
    void TestWorkspace::write_fixture(
        const std::string &fixture, const std::string &target,
        const std::vector<std::pair<std::string, std::string>> &replacements)
    {
        std::string source = read_file(m_root / fixture);
        for (const auto &[needle, value] : replacements) {
            for (size_t pos = 0; (pos = source.find(needle, pos)) != std::string::npos;
                 pos += value.size()) {
                source.replace(pos, needle.size(), value);
            }
        }
        std::ofstream(m_root / target) << source;
    }

    // Run a child command.
    TestCommandResult
    TestWorkspace::run(const std::string &executable, const std::vector<std::string> &arguments,
                       const std::vector<std::pair<std::string, std::string>> &environment)
    {
        std::filesystem::path outPath =
            m_root / ("command-" + std::to_string(m_command++) + ".out");
        std::filesystem::path errPath = m_root / ("command-" + std::to_string(m_command) + ".err");
        std::ostringstream shell;
        shell << "cd " << shell_quote(m_root.string()) << " && ";
        for (const auto &[name, value] : environment) {
            shell << name << '=' << shell_quote(value) << ' ';
        }
        shell << shell_quote(executable);
        for (const std::string &argument : arguments) {
            shell << ' ' << shell_quote(argument);
        }
        shell << " >" << shell_quote(outPath.string()) << " 2>" << shell_quote(errPath.string());

        int raw = std::system(shell.str().c_str());
        int status = raw == -1 ? -1 : WIFEXITED(raw) ? WEXITSTATUS(raw) : 128;
        return {status, read_file(outPath), read_file(errPath)};
    }

    // Run llmc++.
    TestCommandResult
    TestWorkspace::llmcpp(const std::vector<std::string> &arguments,
                          const std::vector<std::pair<std::string, std::string>> &environment)
    {
        return run(LLMCPP_PATH, arguments, environment);
    }

    // Run llmc++ with the scripted agent.
    TestCommandResult TestWorkspace::mock(const std::string &script,
                                          std::vector<std::string> arguments)
    {
        std::filesystem::path log =
            m_root / (std::filesystem::path(script).stem().string() + ".log");
        arguments.insert(arguments.begin(), "-fllm-agent=" + std::string(MOCK_AGENT_PATH));
        return run(LLMCPP_PATH, arguments,
                   {{"LLMCPP_MOCK_SCRIPT", (m_root / script).string()},
                    {"LLMCPP_MOCK_LOG", log.string()}});
    }
}
