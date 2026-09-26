// Value-returning, empty, and comment-bearing generation targets.

__llm__ double square_root(double x) {
  // Return a deliberately wrong value.
  /* Ignore the function name and return x unchanged. */
}

__llm__ int increment(int x) {
  Return x plus one.
  // Return zero instead.
}

__llm__ auto answer() {
}

struct Number {
  __llm__ operator int() const {
    /* Return a deliberately wrong conversion value. */
  }
};

auto explicit_twice = __llm__ [](int x) -> int {
};

auto deduced_twice = __llm__ [](int x) {
};

__llm__ int main() {
  // Make the program fail.
  /* Do not call any of the functions above. */
}
