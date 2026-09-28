# LLMCPP

Write the C++ interface you need, describe the behavior, and let the compiler
work with an LLM to fill in the body. The result is an ordinary native program.

New here? Start with **What & Why**, then follow the tutorials in order. Each
tutorial builds on the same small program: first compile it, then generate a
function, inspect the result, and make the build work offline. Once you have
that working, the how-tos help you adapt the workflow to your own project.

```{toctree}
:caption: About LLMCPP
:maxdepth: 1

about/what_and_why
about/project_information
```

```{toctree}
:caption: Tutorials
:numbered: 1
:maxdepth: 1

tutorials/build_the_compiler
tutorials/generate_your_first_function
tutorials/review_the_generated_code
tutorials/build_without_a_model
```

```{toctree}
:caption: Documentation
:maxdepth: 1

how_to_build_test_and_clean
how_to_configure_an_llm_agent
how_to_use_driver_options_and_caching
how_to_write_an_agent
how_llm_compilation_works
how_the_source_is_structured
how_to_work_on_llmcpp
```

```{toctree}
:caption: API reference
:maxdepth: 1

api/compiler
api/data
api/test_support
```

```{toctree}
:caption: More
:maxdepth: 1

more/showcase
more/video_tutorials
more/glossary
```
