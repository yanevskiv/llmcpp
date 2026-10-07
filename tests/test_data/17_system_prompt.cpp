// Replace the default instructions for this target.
__llm__(system_prompt("target.md")) int answer()
{
    Return 42.
}

// Keep the driver instructions for this target.
__llm__ int increment(int x)
{
    Return x plus one.
}
