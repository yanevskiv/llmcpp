/*
 * C++ file for testing return values and comment-bearing targets.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * llmc++ is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * llmc++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with llmc++; if not, see
 * <https://www.gnu.org/licenses/>.
 */

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
