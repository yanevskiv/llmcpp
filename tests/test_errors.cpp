/*
 * C++ file for testing invalid generation annotations.
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
