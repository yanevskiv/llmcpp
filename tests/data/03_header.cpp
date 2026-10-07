// __llm__ in an included header is an error: only the main file is rewritten.
#include "include/03_header.h"

// Include the annotated header through a normal source file.
int main()
{
    from_header();
}
