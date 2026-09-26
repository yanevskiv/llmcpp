#include <iostream>

template <typename T> __llm__ void describe_type()
{
    Print a friendly description of the concrete type T followed by a newline.
}

int main()
{
    describe_type<int>();
    describe_type<double>();
}
