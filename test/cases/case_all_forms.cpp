// Every supported form of generated function. Bodies come from json/case_all_forms.json.
#include <iostream>
#include <string>
#include <vector>

// Adds up the numbers.
__llm__ void sum(const std::vector<int>& values, int& total) {
    Store the sum of values in total.
}

struct Counter {
    int count = 0;
    std::string name;

    __llm__ Counter(std::string n) : name(std::move(n)) {
        Print "created <name>" on its own line.
    }

    __llm__ void bump() {
        Increment count by one.
    }

    void report() const;

    __llm__ ~Counter() {
        Print "destroyed <name>" on its own line.
    }
};

__llm__ void Counter::report() const {
    Print "<name>: <count>" on its own line.
    Balanced braces in prompts are fine: { }.
    "Strings may contain unmatched braces: {{{".
    // Comments may contain unmatched braces too: }}}
}

template <typename T>
__llm__ void print_twice(const T& value) {
    Print value twice, separated by a space, then a newline.
}

static __llm__ void greet() {
    Print "hello" on its own line.
}

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
    auto twice = __llm__ [&](int x) {
        Set doubled to x * 2.
    };
    twice(21);
    std::cout << "doubled " << doubled << "\n";
    return 0;
}
