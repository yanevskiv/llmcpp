// The mock agent submits code that doesn't compile, then gives up.
__llm__ void f() {
    Do something impossible.
}

int main() { f(); }
