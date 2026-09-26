/*
 * C++ header for the llmc++ driver application.
 */

#ifndef LLMCPP_DRIVER_APP_H
#define LLMCPP_DRIVER_APP_H

#include <string>
/** Namespace for llmcpp public interfaces. */
namespace llmcpp
{
    /** Namespace for llmcpp driver declarations. */
    namespace driver
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
}

#endif
