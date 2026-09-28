// Add both driver and function context files.
__llm__(context("local.md"), context("global.md"), max_output_tokens(512)) int answer()
{
    Return 42.
}
