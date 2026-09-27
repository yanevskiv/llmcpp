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
