// Every misuse of __llm__ that llmc++ diagnoses; all of them are reported.
#include <string>

__llm__ void directive() {
  Prompt.
#if 1
  More prompt.
#endif
}

__llm__ void declaration_only();

__llm__ constexpr void compile_time() {
  Do nothing.
}

__llm__ void try_block() try {
  Do something.
} catch (...) {
}

struct S {
  __llm__ S() = default;
};

#define WRAP __llm__
WRAP void via_macro() { /* Prompt. */ }

__llm__ int not_a_function = 5;

int main() {}
