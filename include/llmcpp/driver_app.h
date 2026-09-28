/*
 * C++ header for the llmc++ driver application.
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

#ifndef LLMCPP_DRIVER_APP_H
#define LLMCPP_DRIVER_APP_H

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Coordinates command-line parsing, generation, and Clang driver execution. */
    class DriverApp
    {
    public:
        /**
         * Bind the driver application to process arguments.
         *
         * @param argc Number of process arguments.
         * @param argv Process argument vector.
         */
        DriverApp(int argc, char **argv);
        /**
         * Run the llmc++ driver.
         *
         * @return Process exit status.
         */
        int run();

    private:
        /**
         * Resolve the absolute path of the running executable.
         *
         * @return Absolute executable path.
         */
        std::string executable_path() const;
        /**
         * Anchor executable-path discovery to this binary.
         */
        static void anchor();
        /** Process argument count. */
        int m_argc;
        /** Process argument vector. */
        char **m_argv;
    };

}

#endif
