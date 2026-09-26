/*
 * C++ header for a person used by the projection example.
 */
#ifndef EXAMPLES_EXAMPLE12_PERSON_H
#define EXAMPLES_EXAMPLE12_PERSON_H

#include <string>
/** Structure for a named person. */
struct Person
{
    /** Person's display name. */
    std::string m_name;
    /** Person's age in years. */
    unsigned m_age;
};

#endif
