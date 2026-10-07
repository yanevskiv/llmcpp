// The mock agent submits code that doesn't compile, then gives up.
// Request a body from an agent that will report failure.
__llm__ void f() {
    Do something impossible.
}

// Exercise the generated entry point when generation succeeds.
int main()
{
    f();
}
