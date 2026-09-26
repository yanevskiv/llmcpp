#include <iostream>
#include <vector>

/*
 * Potential result:
 *   void a()
 *   {
 *       std::cout << "LLM says hello from a()\n";
 *   }
 */
__llm__ void a()
{
    Use std::cout to print: "LLM says hello from a()"
}

struct SomeClass
{
    /*
     * Potential result:
        void b()
        {
            std::cout << "LLM says hello from SomeClass::b()";
        }

    */
    __llm__ void b() {
        You have std::cout available.
        Your job is to use it to print the following:
        "LLM says hello from <class>:<method>"
    }
};

__llm__ void c()
{
    // {{{}}}}}}}}} Braces in comments do not affect body matching.
    This is fine because an LLM can read plain text.
    Message for the LLM: just do nothing (output no code).
    "Braces in strings do not count either: {{{}}}}}}}"
}

__llm__ void d(const std::vector<int>& vec)
{
    Sum the values in vec and print the result.
}

int main()
{
    a();
    SomeClass{}.b();
    c();
    (__llm__ [](){
        This is an immediately invoked lambda.
        Use std::cout to print "LLM says hello from IIFE in main()".
    })();
    return 0;
}
