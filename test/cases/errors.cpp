// Every misuse of __llm__ that llmc++ diagnoses; all of them are reported.
#include <string>

__llm__ int non_void() {
  Return 42.
}

__llm__ auto deduced() {
  Do something.
}

__llm__ void no_prompt() {
}

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
  __llm__ operator int() { Convert to an integer. }
};

#define WRAP __llm__
WRAP void via_macro() { /* Prompt. */ }

__llm__ int not_a_function = 5;

int main() {
  auto l = __llm__ []() -> int {
    Return 1.
  };
  (void)l;
}
