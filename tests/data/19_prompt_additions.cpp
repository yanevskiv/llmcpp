// Combine target instructions, configuration, and a transcript.
__llm__(append_prompt("first.md"), system_prompt("base.md"), append_prompt("second.md"),
        agent_config("target.json"), transcript("target.jsonl")) int answer()
{
    Return 42.
}

// Keep the driver defaults for the second target.
__llm__ int increment(int x)
{
    Return x plus one.
}
