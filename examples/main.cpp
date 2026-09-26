#include <iostream>

template <typename T> __llm__ void f()
{
    Use std::cout to print what is T here.
}

int main()
{
    f<int>();
    f<short>();
}
