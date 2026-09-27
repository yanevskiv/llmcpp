// __llm__ in an included header is an error: only the main file is rewritten.
#include "include/test_header.h"

int main() { from_header(); }
