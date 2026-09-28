/*
 * C++ file for fake openai server test support.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

// Headers for integration-test support and its dependencies.
#include "llmcpp/test/test_fake_openai_server.h"

#include <stdexcept>
// Namespace for llmcpp integration-test support.
namespace llmcpp::test
{
    // Initialize the scripted model server.
    TestFakeOpenAIServer::TestFakeOpenAIServer()
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
                    std::to_string(call) + R"json(","type":"function","function":{"name":")json" +
                    name + R"json(","arguments":")json" + arguments + R"json("}}]}}]})json",
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

    // Stop and clean up the scripted model server.
    TestFakeOpenAIServer::~TestFakeOpenAIServer()
    {
        m_server.stop();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // Get the local API URL.
    std::string TestFakeOpenAIServer::base_url() const
    {
        return "http://127.0.0.1:" + std::to_string(m_port);
    }

    // Get the request count.
    unsigned TestFakeOpenAIServer::calls() const
    {
        return m_calls;
    }

    // Get captured requests.
    std::vector<std::string> TestFakeOpenAIServer::requests() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_requests;
    }

    // Get the request validation diagnostic.
    std::string TestFakeOpenAIServer::problem() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_problem;
    }
}
