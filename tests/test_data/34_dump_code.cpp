// Request body output only for this target.
__llm__(dump_code) int answer()
{
    Return 42.
}

// Keep the other target silent.
__llm__ int increment(int x)
{
    Return x plus one.
}
