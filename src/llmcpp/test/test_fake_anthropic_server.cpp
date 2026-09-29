/*
 * C++ file for fake anthropic server test support.
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
#include "llmcpp/test/test_fake_anthropic_server.h"

#include <stdexcept>
// Namespace for llmcpp integration-test support.
namespace llmcpp::test
{
    // Initialize the scripted model server.
    TestFakeAnthropicServer::TestFakeAnthropicServer()
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
                std::string(R"json({"model":"native-test-model","content":[)json") + content + "]}",
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

    // Stop and clean up the scripted model server.
    TestFakeAnthropicServer::~TestFakeAnthropicServer()
    {
        m_server.stop();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // Get the local API URL.
    std::string TestFakeAnthropicServer::base_url() const
    {
        return "http://127.0.0.1:" + std::to_string(m_port);
    }

    // Get the request count.
    unsigned TestFakeAnthropicServer::calls() const
    {
        return m_calls;
    }

    // Get captured requests.
    std::vector<std::string> TestFakeAnthropicServer::requests() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_requests;
    }

    // Get the request validation diagnostic.
    std::string TestFakeAnthropicServer::problem() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_problem;
    }
}
