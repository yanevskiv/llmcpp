// Exercises the tool server through the mock agent (see json/06_tools.json).
#include <iostream>

// Expose public and private members to compiler tools.
class Account {
public:
    // Deposit an amount through a generated method.
    void deposit(int amount);
    // Read the private balance through an ordinary method.
    int balance() const
    {
        return cents;
    }

private:
    int cents = 0;
};

// Generate a method that can access private state.
__llm__ void Account::deposit(int amount) {
    Add amount to cents.
}

// Ask the agent to inspect an account from outside the class.
__llm__ void peek(Account& account) {
    Print the balance on its own line.
}

// Keep a declaration after generated functions for context queries.
void helper_declared_later()
{
}

// Exercise the generated methods and lambda.
int main() {
    Account account;
    account.deposit(5);
    peek(account);
    int hits = 0;
    // Capture and increment the local hit counter.
    auto count = __llm__ [&]() {
        Increment hits.
    };
    count();
    std::cout << hits << "\n";
}
