#include <iostream>

void f()
{
    // use std::cout to print hello world
    // llmcpp: generated (model=claude-opus-5-5, key=5fe0896fc81d02686e3353996dd5d80658bb31345633fc3b90a66203db797b6a)
    std::cout << "hello world" << std::endl;
}

void g()
{
    // version: 1.0.0
    // key: f76461ae871a712f4ae8ef88a87ba76a4ae335cf1f5df1c2631dc92b4c1ca5b8
    // context: 2e1125bc3c9391cfde10db8317125354f92d5d05f27835c2c6437f09cff35aa7
    // system_prompt: 817e1e746bad2226900ca9fa6774450a3b342ec7fffa920e44cc26b6e39fd50f
    // policy: {"cache":"enabled","max_attempts":4,"max_tool_calls":60,"model":"","timeout_seconds":600}
    // cache_salt: ""
    // agent_config: 44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a
    // agent: claude
    // function: void g()
    // location: main.cpp:8:1
    // model: claude-opus-5-5
    // date: 2026-09-27T17:57:22Z
    // prompt:
    //   call f() 3 times
    // ---
    for (int i = 0; i < 3; ++i) {
        f();
    }
}


int main() 
{
    return 0;
}
