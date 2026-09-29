/*
 * C++ header for fake openai server test support.
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

#ifndef LLMCPP_TEST_FAKE_OPENAI_SERVER_H
#define LLMCPP_TEST_FAKE_OPENAI_SERVER_H

#include <httplib.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
/** Namespace for llmcpp integration-test support. */
namespace llmcpp::test
{
    /** Class for scripted model API responses. */
    class TestFakeOpenAIServer
    {
    public:
        /** Initialize the scripted model server. */
        TestFakeOpenAIServer();
        /** Stop and clean up the scripted model server. */
        ~TestFakeOpenAIServer();
        /**
         * Get the local API URL.
         * @return Loopback URL including its port.
         */
        std::string base_url() const;
        /**
         * Get the request count.
         * @return Number of model requests.
         */
        unsigned calls() const;
        /**
         * Get captured requests.
         * @return Request bodies in arrival order.
         */
        std::vector<std::string> requests() const;
        /**
         * Get the request validation diagnostic.
         * @return Empty when requests were valid.
         */
        std::string problem() const;

    private:
        /** HTTP server for scripted responses. */
        httplib::Server m_server;
        /** Bound loopback port. */
        int m_port = -1;
        /** HTTP listener thread. */
        std::thread m_thread;
        /** Number of received model requests. */
        std::atomic<unsigned> m_calls{0};
        /** Mutex guarding requests and diagnostics. */
        mutable std::mutex m_mutex;
        /** Captured request bodies. */
        std::vector<std::string> m_requests;
        /** Request validation diagnostic. */
        std::string m_problem;
    };
}

#endif
