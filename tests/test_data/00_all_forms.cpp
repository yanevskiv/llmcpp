// Every supported form of generated function. Bodies come from json/00_all_forms.json.
#include <iostream>
#include <string>
#include <vector>

// Adds up the numbers.
__llm__ void sum(const std::vector<int>& values, int& total) {
    Store the sum of values in total.
}

// Exercise generated constructors, methods, and destructors.
struct Counter {
    int count = 0;
    std::string name;

    // Announce construction.
    __llm__ Counter(std::string n) : name(std::move(n)) {
        Print "created <name>" on its own line.
    }

    // Increase the count.
    __llm__ void bump() {
        Increment count by one.
    }

    // Report the current count.
    void report() const;

    // Announce destruction.
    __llm__ ~Counter() {
        Print "destroyed <name>" on its own line.
    }
};

// Generate an out-of-line method body.
__llm__ void Counter::report() const {
    Print "<name>: <count>" on its own line.
    Balanced braces in prompts are fine: { }.
    "Strings may contain unmatched braces: {{{".
    // Comments may contain unmatched braces too: }}}
}

// Generate a function template body.
template <typename T>
__llm__ void print_twice(const T& value) {
    Print value twice, separated by a space, then a newline.
}

// Generate a function with internal linkage.
static __llm__ void greet() {
    Print "hello" on its own line.
}

// Exercise each generated form in one program.
int main() {
    std::vector<int> v{1, 2, 3, 4};
    int total = 0;
    sum(v, total);
    std::cout << "total " << total << "\n";
    {
        Counter c("widget");
        c.bump();
        c.bump();
        c.report();
    }
    print_twice(7);
    greet();
    int doubled = 0;
    // Capture local state in a generated lambda.
    auto twice = __llm__ [&](int x) {
        Set doubled to x * 2.
    };
    twice(21);
    std::cout << "doubled " << doubled << "\n";
    return 0;
}
