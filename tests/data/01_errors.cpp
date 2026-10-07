// Every misuse of __llm__ that llmc++ diagnoses; all of them are reported.
#include <string>

// Reject preprocessor directives inside prompts.
__llm__ void directive()
{
    Prompt.
#if 1
    More prompt.
#endif
}

// Reject an annotated declaration without a body.
__llm__ void declaration_only();

// Reject compile-time generation.
__llm__ constexpr void compile_time()
{
    Do nothing.
}

// Reject function try blocks.
__llm__ void try_block() try
{
    Do something.
}
catch (...)
{
}

// Reject defaulted annotated constructors.
struct S
{
    // Reject a generated defaulted constructor.
    __llm__ S() = default;
};

// Reject annotations introduced by macro expansion.
#define WRAP __llm__
// Trigger the macro-expansion diagnostic.
WRAP void via_macro()
{
    /* Prompt. */
}

// Reject annotations on non-functions.
__llm__ int not_a_function = 5;

// Provide an entry point for the diagnostic fixture.
int main()
{
}
