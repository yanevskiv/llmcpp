// Override the model and output limit for one target.
__llm__(model("function-model"), max_output_tokens(512)) int f()
{
}
