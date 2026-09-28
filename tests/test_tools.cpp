/*
 * C++ file for testing agent tools.
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

// Exercises the tool server through the mock agent (see json/test_tools.json).
#include <iostream>

class Account {
public:
    void deposit(int amount);
    int balance() const { return cents; }

private:
    int cents = 0;
};

__llm__ void Account::deposit(int amount) {
    Add amount to cents.
}

__llm__ void peek(Account& account) {
    Print the balance on its own line.
}

void helper_declared_later() {}

int main() {
    Account account;
    account.deposit(5);
    peek(account);
    int hits = 0;
    auto count = __llm__ [&]() {
        Increment hits.
    };
    count();
    std::cout << hits << "\n";
}
