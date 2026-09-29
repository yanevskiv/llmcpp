/*
 * C++ header for a task used by the ranking example.
 */
#ifndef EXAMPLES_EXAMPLE11_TASK_H
#define EXAMPLES_EXAMPLE11_TASK_H

#include <string>
/** Structure for a prioritized task. */
struct RankedTask
{
    /** Human-readable task name. */
    std::string m_name;
    /** Task priority, with greater values ranked first. */
    int m_priority;
};

#endif
