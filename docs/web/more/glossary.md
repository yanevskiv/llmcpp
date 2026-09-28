# Glossary

**Agent**
: The component that uses a model and compiler tools to produce a function body.
  It can be a native API client or an external process.

**Backend**
: The API or CLI used for generation, such as Anthropic, OpenAI, Codex,
  or Claude Code.

**Body cache**
: Saved accepted C++ bodies and metadata used to avoid repeating generation.
  A matching entry is not proof of correctness.

**Compiler context**
: Declarations, types, members, and other information an agent can inspect
  through Clang-backed tools.

**Modifier**
: The `__llm__` marker, optionally followed by per-function options.

**Offline build**
: A build that requires cached bodies and never contacts an agent.

**Prompt**
: The instructions written inside an annotated body. Comments are ignored.

**Submission**
: A candidate body offered through the `submit` compiler tool for validation
  and acceptance.

**System prompt**
: Instructions shared across generation tasks, distinct from each function's
  body prompt.

**Translation unit**
: A source file together with the headers it includes. Annotations currently
  must be written in its main source file.
