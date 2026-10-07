// Pin one target to a separate cache directory and key.
__llm__(cache_dir("custom-cache"), key("abcdef0123")) int answer()
{
    Return 42.
}

// Let the other target use the driver cache directory.
__llm__ int increment(int x)
{
    Return x plus one.
}
