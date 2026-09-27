# llmcpp — PLAN

A fork of clang++ that understands an `__llm__` function specifier. An `__llm__`
function, method or lambda may return any valid C++ return type, and its body is
**plain-language text**.
That text is the prompt. While compiling, clang asks an LLM agent to write the
real body. The agent never sees the source text. It learns about the surroundings by
calling tools that query clang's AST and `Sema` at the exact point where the body
appears.

```cpp
__llm__ void greet(const std::string& name) {
    Use std::cout to print "Hello, <name>!" followed by a newline.
}
```

---

## 1. Goals and non-goals

**Goals**
- `__llm__` on free functions, member functions (inline and out-of-line), constructors,
  destructors, and lambdas with ordinary explicit or deduced return types.
- Before ordinary C++ parsing, the compiler blanks prompt bodies in a
  length-preserving view. Source locations and the surrounding compilation context
  therefore remain unchanged while prompt text does not need C++ comment syntax.
- The generated code is compiled as if the user had written it at that spot. Name
  lookup, access checks, `this` and captures all behave normally.
- The LLM can see only what the compiler can see at that point, exposed through tools.
- Builds can be reproduced. A lockfile/cache means CI and teammates don't need network
  access or get different code.
- Errors are clear: every diagnostic in generated code points both to the generated
  text and to the `__llm__` declaration.
- An inspectable intermediate step: `llmc++ --llm main.cpp` writes `main.llm.cpp`,
  plain C++ with the generated bodies filled in, which any C++ compiler can build
  (§3.6). This works like `nvcc --cuda main.cu` producing `main.cu.cpp.ii`.

**Non-goals (v1)**
- Letting the LLM change anything outside the body (new globals, new includes,
  changed signatures).
- Mixing real code with the prompt (partly written bodies).
- Generating code separately for each template instantiation (v1 generates only the
  pattern; see §8).
- C, Objective-C, or CUDA interaction.

---

## 2. Surface syntax

Language option `-fllm` (off by default, like `-fcuda`). When enabled, `__llm__` is a
keyword.

```cpp
__llm__ void f(int x) { Use x. }                        // free function
struct S { __llm__ void g() const { Update this object. } }; // inline member
__llm__ void S::h() { Do the work. }                    // out-of-line member
__llm__ S::S() : x{1} { Finish initialization. }        // constructor
__llm__ S::~S() { Release owned resources. }            // destructor
auto l = __llm__ [&](int y) { Add y to the total. };     // lambda
```

Rules:
- Non-comment text between the body's outer `{ ... }` is the prompt. C++ line and
  block comments are ignored and are never supplied to the agent.
- A body without prompt text is valid. The agent infers conventional behavior from
  the name, signature, parameters, return type, and compiler context.
- Preprocessor directives (`#if`, `#define`, …) and macro expansions inside the body
  are errors in v1. Otherwise `#if 0` could silently drop part of the prompt.
- A lexical brace counter locates the body boundary. Braces in ordinary prompt text
  must balance; braces inside comments, string literals, character literals, or raw
  string literals are ignored.
- Everything outside the body (signature, default arguments, constructor init list)
  is ordinary C++.
- `__llm__` isn't allowed on a declaration without a body, on `= default`/`= delete`,
  or inside a macro expansion (v1).
- A function can't be both `__llm__` and `constexpr` in v1 (see §8).

### 2.1 Return types

Generated functions may use any return type accepted by C++, including `auto`,
`decltype(auto)`, conversion operators, and lambdas with explicit or deduced return
types. Candidate bodies are inserted into the original declaration before shadow
compilation, so Clang checks explicit returns and performs ordinary return-type
deduction. Constructors and destructors continue to have no return value.

`get_task` reports the explicit return type or says that it is deduced. It also
reports writable reference/pointer parameters, `*this`, reference captures, and
globals through which a function may produce additional results (§4).

---

## 3. Compiler architecture

### 3.1 Lexing

The prototype recognizes `__llm__` before ordinary C++ parsing, locates the outer
braces of each annotated function or lambda, and replaces every non-newline prompt
character with a space in a parsing-only buffer. This makes arbitrary prose invisible
to Clang while preserving every source offset. The original buffer remains the source
of prompt text and generated-output rewrites.

### 3.2 Collecting the prompt

After Clang identifies the body range in the parsing view, the same byte offsets select
the prompt from the original source. C++ line and block comments are removed before
common indentation and outer blank lines are normalized. A comment-only body
therefore produces an empty prompt and relies on declaration-based inference.

### 3.3 Parsing and validation

- `Parser::ParseDeclarationSpecifiers` accepts `__llm__` and records it in `DeclSpec`
  (`isLLMSpecified()`).
- `Parser::ParseLambdaExpression` accepts `__llm__` before the lambda introducer.
- `Sema` checks the declaration when it's created (`ActOnFunctionDeclarator`, and
  lambdas after the call operator is built). Return checking and deduction happen
  when the generated candidate is shadow-compiled (§2.1).
- The empty parsing-view body supplies the declaration and exact source range. The
  pass then collects the original non-comment prompt (§3.2), rejects a
  preprocessor directive, and runs generation (§3.5).
- The AST records the fact with an implicit `LLMGeneratedAttr(promptText, model,
  cacheKey)` on the `FunctionDecl`/`CXXMethodDecl`. `-ast-dump` and tooling can then
  tell which bodies were generated.

### 3.4 When to generate: synchronously, at the parse point (recommended)

Generation happens **at the moment the parser reaches the body**, while `Sema`'s
`Scope` chain, `DeclContext`, `this` type, lambda captures and template parameters
are all live. This is the main design decision, and it makes the hard parts easy:

- Name lookup from the tools matches exactly what real code at that spot would see.
  Free functions see only earlier declarations. Inline member bodies see the complete
  class, because clang already parses them after the closing `}` of the class, so
  generation happens then too.
- Lambda captures are handled correctly. With `[&]`/`[=]`, a variable is captured
  only if the generated body uses it, which is what normal parsing does.
- Injecting the result is easy. Put the generated text in a virtual `MemoryBuffer`
  (`<llm:ns::f>`), lex it into tokens, push them with `Preprocessor::EnterTokenStream`,
  and call the normal `ParseFunctionStatementBody`. The result is a real
  `CompoundStmt` with real source locations.

The cost is that generation runs one at a time within a TU. Parallelism comes later
from a pre-pass plus the cache (§6.3).

Rejected alternatives:
- *Generate at end of TU (`ActOnEndOfTranslationUnit`)*: scopes are gone and would
  have to be rebuilt the way `-fdelayed-template-parsing` does it. That is fragile,
  especially for lambda captures.
- *A standalone source-rewriting libTooling tool*: fine for a quick prototype (see
  M0.5), but it can't see live `Sema` scope.

### 3.5 Generation loop (inside clang)

```
ParseLLMFunctionBody(FD):
  prompt = collectPromptFromOriginalSource(FD's body range) // §3.2
  ctx    = LLMContext::capture(Sema, CurScope, FD)      // §4
  key    = cacheKey(prompt.text, ctx.fingerprint(), model)   // §6
  if code = cache.lookup(key): goto inject
  if -fllm-offline: error "no cached body for __llm__ function 'f'"; stop
  code   = agent.run(prompt, ctx, tools = LLMToolServer(Sema, CurScope, FD))
  cache.store(key, code)
inject:
  buffer = SourceManager.createVirtualFile("<llm:" + qualName + ">", code)
  lex + EnterTokenStream; body = ParseFunctionStatementBody
  if errors: note "in code generated for __llm__ function declared here"
```

If the agent gives up or errors out, clang emits
`error: LLM failed to generate body for 'f'`, attaches the agent's last attempt and
diagnostics as notes, and stops before compiling or emitting rewritten source.

### 3.6 Driver: `llmc++` and the `--llm` intermediate output

**`llmc++`** is the clang driver installed under another name, the same way `clang++`
and `clang-cl` are. The driver recognizes the name from `argv[0]`
(`clang/lib/Driver/ToolChain.cpp`, `getDriverMode`/`ParsedClangName`), behaves like
`clang++`, and turns on `-fllm` by default.

| Command | Result |
|---|---|
| `llmc++ main.cpp` | Full build to `a.out`. Generation and compilation happen in one process; no intermediate file is written. |
| `llmc++ --llm main.cpp` | Stops after generation and writes **`main.llm.cpp`**. No object file, no linking. |
| `llmc++ --llm main.cpp -o gen/out.cpp` | Same, to the given path. |
| `llmc++ --llm a.cpp b.cpp` | One output per input: `a.llm.cpp`, `b.llm.cpp` (`-o` isn't allowed with more than one input, as with `-E`). |
| `llmc++ main.llm.cpp` / `g++ main.llm.cpp` | Builds the intermediate file like any C++ source. |

**How `--llm` works.** `--llm` selects a new frontend action,
`EmitLLMSourceAction` (next to `PrintPreprocessedAction`/`SyntaxOnlyAction` in
`clang/lib/Frontend/FrontendActions.cpp`, flag in `Options.td`). It runs the normal
parse (so generation, validation and `try_compile` all happen as in §3.3–§3.5), then
at the end of the TU uses a `clang::Rewriter` on the **original, unpreprocessed**
main file:
- Every `__llm__` keyword is removed. `LLMGeneratedAttr` stores the keyword's
  location for this.
- Every `__llm__` body `{ ... }` is replaced by the prompt rendered as comments,
  followed by the generated statements:

  ```cpp
  // before (main.cpp)
  __llm__ void greet(const std::string& name) {
      Use std::cout to print "Hello, <name>!" followed by a newline.
  }

  // after (main.llm.cpp)
  void greet(const std::string& name) {
      // Use std::cout to print "Hello, <name>!" followed by a newline.
      // llmcpp: generated (model=<id>, key=<hash>)
      std::cout << "Hello, " << name << "!\n";
  }
  ```
- Everything else (includes, macros, comments, formatting) is copied unchanged, so a
  diff between `main.cpp` and `main.llm.cpp` shows only the generated code.
- If generating any body fails, the output isn't written (as with a compile error),
  and the exit status is non-zero.
- The output is written only if its content changed, which avoids needless rebuilds
  under CMake build backends such as Make or Ninja.

**Why not preprocessed output like nvcc's `.ii`?** Unpreprocessed output is readable,
reviewable, and can be built with other compilers. The trade-off is that
`main.llm.cpp` must be built with the same include paths and macros as `main.cpp`,
since the bodies were checked under them. `--llm -E` gives an nvcc-style preprocessed
`main.llm.ii` when a fully self-contained file is needed.

**Headers.** `main.llm.cpp` still `#include`s the original headers. If one of them
contains `__llm__` functions, the intermediate file isn't plain C++. In v1, `--llm`
therefore reports `error: __llm__ function in included header 'foo.h'; --llm only
rewrites the main file`. A v2 option `--llm-headers=<dir>` would write `foo.llm.h`
files and change the `#include`s in the output to point to them.

**Build-system use.** Two-step builds work with any compiler. For example, a
CMake project can model the generated source with a custom command:

```cmake
add_custom_command(
  OUTPUT main.llm.cpp
  COMMAND llmc++ --llm ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
          -o ${CMAKE_CURRENT_BINARY_DIR}/main.llm.cpp
  DEPENDS main.cpp
)
add_executable(app ${CMAKE_CURRENT_BINARY_DIR}/main.llm.cpp)
```

Committing `*.llm.cpp` files is an alternative to committing `.llmcache/` (§6.2) for
teams that want the generated code in the tree.

---

## 4. What the LLM can see: the tool server

The agent works through tools. Clang implements them in-process (`LLMToolServer`)
against `Sema`, using the captured `Scope*`. Every result is derived from the AST
(pretty-printed declarations, types, locations, attached comments). Raw source text of
other functions is never returned; only declarations/signatures are exposed.

| Tool | Returns |
|---|---|
| `get_task()` | Prompt (`text` and `raw`, §3.2); the target's doc comment if it has one; full signature (qualified name, params, cv/ref qualifiers, `noexcept`); constructor init list if any; whether it's a lambda and its captures; and the **outputs**: non-const reference/pointer parameters, whether `*this` is modifiable, and reference captures. |
| `get_context()` | Chain of enclosing `DeclContext`s (namespaces, classes, enclosing function for lambdas), `this` type, template parameters, and **local variables in scope** with their types (for lambdas). |
| `lookup(name)` | `Sema::LookupName` / qualified lookup from the current scope. Returns each declaration found: kind, pretty-printed declaration, access, and whether it's usable here. Answers questions like "is `std::cout` available?" |
| `list_members(type)` | Fields, methods (with overloads), bases and nested types, with access as seen from the target. |
| `describe_type(type)` | Canonical type, size/align if complete, whether it's copyable/movable, key operators found by lookup (e.g. `operator<<` with `std::ostream`). |
| `list_namespace(ns, filter?)` | Names declared in a namespace so far (paginated). Used to explore `std::` etc. |
| `get_comment(name)` | The comment attached to a declaration, as raw text plus a parsed brief/params/returns summary (§4.2). |
| `included_headers()` | The include tree so far (names only). Lets the agent check, for example, that `<iostream>` is included and `<format>` isn't. |
| `try_compile(body)` | Compiles a candidate body in this exact context and returns diagnostics. The key tool for self-correction (§4.1). |
| `submit(body)` | Final answer: body statements only, without the signature or outer braces. May be empty. |

A typical sequence for a member function whose prompt asks it to print the class and
method name:
1. `get_task` → the qualified name is `Widget::describe`.
2. `lookup("std::cout")` → found (`extern ostream cout`).
3. `try_compile('std::cout << "Widget::describe\n";')` → no diagnostics.
4. `submit`.

### 4.0 Code inside the prompt

Prompt prose can include code as a sketch:

```cpp
__llm__ void print_sum(const std::vector<int>& vec) {
    Follow this sketch, filling in the TODOs:
    for (int i = 0; i < vec.size(); i++) {
        // TODO: sum vec up
    }
    // TODO: print sum
}
```

Rules for the agent:
- Treat sketched code as the **intended structure**: follow it, fill in the TODOs, and
  fill in missing declarations (here the accumulator).
- The agent may fix code that doesn't compile or is plainly wrong. For example,
  comparing `int i` with `vec.size()` gives a signed/unsigned warning, and the agent
  may change `i` to `std::size_t`. It must not change what the code does.
- `try_compile` diagnostics are the main check that the result is real C++.

### 4.1 Implementing `try_compile`

Rolling back `Sema` state after a failed parse isn't safe: template instantiations
and invalid declarations leak. v1 therefore does a **shadow compile**:
- Build a copy of the current TU source. The target body is replaced by the candidate.
  Bodies of `__llm__` functions already generated are replaced by their generated
  code; later `__llm__` bodies are replaced by empty bodies, and `__llm__` is
  defined away (`-D__llm__=`).
- Run `-fsyntax-only` in a new `CompilerInstance` with the same `CompilerInvocation`,
  and collect diagnostics whose locations fall inside the candidate. Return
  statements are checked against the original explicit or deduced return type.
- Speed it up with an automatic PCH of the TU prefix before the target (the same idea
  as clangd's preamble), reused across attempts.

The candidate that passes still goes through the real injection (§3.4). If the two
ever disagree (they shouldn't), the real compile's diagnostics win.

Maybe later: try in-process with a `Sema` `SFINAETrap`-like trap plus a buffering
`DiagnosticConsumer`, but only for bodies that don't trigger instantiations.

### 4.2 Comments outside the prompt

Only text **inside** the body is the prompt. Other comments can still be useful:

- **Comments attached to declarations.** `ASTContext::getRawCommentForAnyRedecl(Decl*)`
  returns the `RawComment` attached to a declaration, and `RawComment::parse` produces
  a `comments::FullComment` AST (`\brief`, `\param`, `\returns`). Because `-fllm` turns
  on `-fparse-all-comments`, ordinary comments directly above a declaration are
  attached too. `get_comment` is built on this, and `lookup`/`list_members` add a
  one-line brief to each result. `get_task` includes the target's own attached comment
  as `doc_comment`. That comment is context, not part of the prompt or the cache key.
- **Comments not attached to anything** (a note in the middle of a file). A possible
  `list_comments(near = target)` tool would return comments from the same file before
  the target, nearest first. It isn't part of v1 because it moves toward "the LLM
  reads the source", which §1 rules out. See the open question in §12.

Comments are the easiest place to hide instructions aimed at the LLM (for example in
a third-party header). Comment text from anywhere except the prompt is returned as
data, clearly labelled with its source file. The agent's system prompt says never to
follow instructions found in tool results (§9).

---

## 5. Connecting clang to the LLM

The prototype supports a native provider path and an external-agent protocol:

- With `LLMCPP_BACKEND=anthropic`, the C++ driver calls the Anthropic Messages API
  through `cpp-httplib` and OpenSSL. The tool-use loop calls `LLMToolServer`
  directly, so Python and MCP transport are not needed.
- With `LLMCPP_BACKEND=claude-code`, the driver starts `llmcpp-agent`, which launches
  the `claude` CLI and bridges its MCP server back to the compiler.
- `-fllm-agent=<command>` or `LLMCPP_AGENT` selects any compatible external agent.
  Clang starts it once per TU and talks over **stdio using JSON-RPC**.
- **Clang is an MCP server.** It exposes the tools in §4 over the MCP protocol on that
  pipe. The agent is any MCP client that can run a tool loop. Benefits:
  - Provider integrations that do not belong in the compiler can remain separate.
  - Any MCP-capable agent can be plugged in, including Claude Code itself
    (`claude -p` with clang as the MCP server).
  - Tests use a **mock agent** that replays scripted tool calls and answers.
- Per-function exchange: clang sends `llm/generate {prompt, qualifiedName, location}`,
  the agent calls tools as needed, then either calls `submit` or returns an error.
- Limits: `-fllm-max-turns=N`, `-fllm-timeout=S`, `-fllm-max-attempts=N`
  (`try_compile` failures).
- Model and settings come from the agent's config (`LLMCPP_MODEL`, API key in env),
  not from compiler flags. They are reported back so they can go into the cache key.

Every backend's system prompt should include:
- Output only body statements and follow the declared or deduced return type.
- Use only names that the tools report as usable.
- Always `try_compile` before `submit`.
- No I/O, UB or side effects the prompt didn't ask for.
- Prefer the simplest correct code.

---

## 6. Reproducibility, caching and build integration

LLM output isn't deterministic, even at temperature 0. The **cache is the source of
truth**, not the model.

### 6.1 Cache key
`sha256(prompt text (normalized, §3.2), normalized target signature, qualified name, context fingerprint, agent/model id, llmcpp version)`

The context fingerprint is a hash of the tool results the agent actually used
(lookups, members, types). If a header changes something the agent relied on, the key
changes and the body is generated again. If unrelated headers change, the key stays
the same.

### 6.2 Storage
- `.llmcache/<key>.cpp`: generated body plus a metadata header (model, date, tool
  transcript hash).
- Meant to be **committed**, like a lockfile. Code review sees exactly what the LLM
  wrote.
- Modes: `-fllm-cache=readwrite` (default for dev), `-fllm-offline` (CI: fail
  if missing, no network), `-fllm-regenerate` (ignore cache).

### 6.3 Performance
- `llmcpp-prefill`: runs the compiler over a compile database with `-fllm-collect`,
  finds cache misses, and runs the agents in parallel. Normal builds then only hit the
  cache.
- Generation runs one at a time within a TU, but different TUs build in parallel as
  usual.

### 6.4 ODR
An `inline` or `__llm__` member function in a header is compiled in many TUs. If
their context fingerprints differ (different includes before the header), each TU
could get a *different body* for the same inline function, which is an ODR violation.
Mitigations:
- For functions with external linkage or `inline`, the key ignores the fingerprint and
  uses only (prompt, signature, qualified name). The first body generated wins and is
  reused everywhere. If it fails `try_compile` in another TU, that's an error, not a
  regeneration.
- Emit the prompt/key hash into a section so the linker (or `llmcpp-check`) can detect
  mismatches.

---

## 7. Diagnostics, debugging, tooling

- Generated code lives in virtual files named after the function (`<llm:ns::Widget::describe>`).
  Errors show the generated line plus
  `note: in body generated for __llm__ function declared here`.
- `-fllm-dump`: print each generated body to stderr. `-fllm-dump-context`: print what
  §4's `get_task`/`get_context` would return, without calling any LLM (very useful
  early on).
- Debug info: include the generated source with DWARF 5 `DW_LNCT_LLVM_source` so
  gdb/lldb can step through it.
- `-ast-dump` shows `LLMGeneratedAttr`.
- clangd and clang-format need llmcpp-aware integrations because raw prompts are not
  ordinary C++ tokens. The generated `*.llm.cpp` file works with existing tools.

---

## 8. Semantic edge cases

| Case | v1 behavior |
|---|---|
| Invalid return or incompatible deduction | Rejected by shadow compilation (§2.1). |
| Templates | Generate once for the **pattern**. The code must work for any template argument. The tools report dependent types as dependent. Instantiation errors point to the generated file. v2: optional `__llm__(per_instantiation)`. |
| `constexpr`/`constinit` | Not allowed with `__llm__` in v1 (a simple check). v2: allowed, `try_compile` also checks that it can be evaluated at compile time. |
| Constructors / destructors | Allowed. The member-init list is ordinary code the LLM sees through `get_task`; only the body is generated. |
| Function-try-blocks | Not allowed with `__llm__` (v1). |
| Coroutines | Subject to ordinary candidate shadow-compilation rules. |
| Virtual / override methods | Allowed. `get_task` includes the overridden declaration and its attached comment. |
| Recursive `__llm__` calls (`f` calls `g`, both `__llm__`) | Fine: only declarations are needed. |
| Default args / `noexcept(expr)` | Parsed normally; only the body is a prompt. |
| Lambdas in default arguments / NSDMIs | Not supported in v1 (unusual parse paths). |
| Out-of-line member defined in a `.cpp` | Fine: the context is the definition's scope. |
| Prompt injection via names, strings or comments in headers | The agent's system prompt treats tool output as data. Tools return declarations and attached comments (labelled with their source), never function bodies. |

---

## 9. Security

- The compiler starts a subprocess that uses the network. It stays opt-in (`-fllm`)
  and is off in `-fllm-offline`.
- The agent's only tools are read-only AST queries plus `try_compile`. It has no
  filesystem or shell access through the compiler.
- Generated code is untrusted. Because the cache is committed, it can be reviewed.
  `llmcpp-diff` shows changes to generated bodies between commits.
- The API key lives only in the compiler process environment (or the external
  agent's environment). It never goes into the cache, dumps, or debug info.

---

## 10. Repository layout

```
llmcpp/
  PLAN.md
  llvm-project/          # git submodule pinned to a release tag (e.g. llvmorg-21.x)
  patches/               # optional: exported patch series for rebasing onto new LLVM
  agent/                 # optional Python adapter for Claude Code
  tools/llmcpp-prefill/  # parallel cache warmer
  test/
    lit/                 # clang lit tests (use mock agent)
    mock-agent/          # scripted MCP client for deterministic tests
    e2e/                 # real-LLM tests, opt-in, not in CI
  examples/              # sample programs using __llm__
  scripts/build.sh       # cmake -G Ninja, clang only, Release+Assertions, lld, ccache
                         # (install step adds the llmc++ symlink to clang)
```

Build: `-DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_USE_LINKER=lld -DLLVM_CCACHE_BUILD=ON`.
Budget: first build takes about 30–60 min; later builds are incremental.

---

## 11. Milestones

**M0: Setup.** Add the llvm-project submodule, a build script, and make sure the
unmodified clang builds and passes a hello-world lit test.

**M0.5 (optional, a few days): prototype without a clang fork.** A lightweight scan
creates a length-preserving, parseable view of prompt bodies. A
`PPCallbacks::MacroExpands` hook records where each `__llm__` expands. The tool
matches each one to the next `FunctionDecl`/`LambdaExpr`, collects the original prose
in its body, answers `get_task`/`lookup` through the AST, calls an LLM, and writes a
`main.llm.cpp` in the §3.6 format. Tests prompt quality, the tool set and the output
format before touching clang internals.

**M1: Syntax.** The `llmc++` driver name, `-fllm`, the `__llm__` keyword, parser support for
functions/methods/constructors/destructors/lambdas, return handling, prompt
collection, the validation errors, and `LLMGeneratedAttr`. Without an agent, bodies
stay empty and a warning is given. Lit tests cover
every form in §2 and these cases:
- Braces, quotes and apostrophes inside prompts.
- An empty body.
- Line and block comments ignored inside a body.
- `#if` and a macro inside the body.
- Non-`void`, `auto`, a `void` typedef, a lambda with and without a trailing return
  type, a conversion operator, a coroutine.
- A constructor init list.
- A late-parsed inline member.
- `-E` round-trip.
- A missing body.

**M2: Context and tools.** `LLMContext::capture` and in-process `LLMToolServer` with
`get_task` (including outputs), `get_context`, `lookup`, `list_members`,
`describe_type`, `get_comment`, `included_headers`. `-fllm-dump-context` with
FileCheck tests covering a free function, an inline member, an out-of-line member, a
constructor, and a lambda with captures.

**M3: Agent loop.** Native Anthropic tool loop, MCP-over-stdio server in clang, the
mock agent, injection of
tokens from a virtual buffer, a `try_compile` shadow compile, and the retry/failure
paths. The reference Python agent. `--llm` / `EmitLLMSourceAction` writing
`*.llm.cpp`, including the header error, multiple inputs, `-o`, and not rewriting
unchanged output. Lit tests check the rewritten file against expected output using
the mock agent. Milestone: a sample program in `examples/` with each kind of
`__llm__` function compiles and produces the expected output with a real model.
This works both with `llmc++ main.cpp` and with `llmc++ --llm main.cpp` followed by
`g++ main.llm.cpp`.

**M4: Reproducibility and UX.** `.llmcache`, cache modes, context fingerprinting,
diagnostic notes, `-fllm-dump`, debug info with embedded source.

**M5: Scale and hardening.** `--llm-headers`, `llmcpp-prefill`, ODR key policy plus the checker,
templates, PCH-accelerated `try_compile`, clangd behavior, and bringing in constexpr.

---

## 12. Open questions

1. Should the LLM be allowed to see the **bodies** of other functions (for style or
   helper reuse), or only declarations? The current choice is declarations only.
   The same question applies to comments not attached to a declaration
   (`list_comments`, §4.2).
2. Should the prompt be able to name tools or give hints (`@use std::format`), or stay
   purely natural language?
3. Should `__llm__` bodies be allowed to call helpers the LLM itself declares
   (generating more than the body)? This is out of scope for v1 and would need
   declaration injection at namespace scope.
4. Should `*.llm.cpp` outputs (§3.6) or `.llmcache/` be the recommended thing to
   commit, or both?

---

## 13. System prompts and a public agent interface

Implemented: prompt files, model requests, agent JSON configuration, version 1
stdio generation protocol, redacted transcripts and replay, and a standalone
Python chat-server adapter example. Persistent socket connections remain future
work as described in §13.2. The optional generic adapter is an example, rather
than a required compiler backend.

The compiler should own the system instructions given to a generation agent.
They are part of the generation contract, not a private implementation detail
of one backend.

### 13.1 System-prompt files

Add a system-prompt option whose argument is a UTF-8 file:

```sh
llmc++ -fllm-system-prompt=PROMPT.md main.cpp
```

It replaces the built-in prompt. Provide a separate composition option:

```sh
llmc++ -fllm-append-system-prompt=PROJECT_RULES.md main.cpp
```

Appending preserves the default tool-use and safety contract while allowing a
project to add its own rules. Do not accept raw prompt text on the command
line: shell quoting is awkward and command lines can expose its contents.

The resolved prompt, including appended files, must be hashed into the cache
identity and recorded in cache metadata. A generated body must not be reused
after the instructions that produced it have changed.

### 13.2 Agent protocol

Make the existing external-agent interface a documented, versioned public
protocol. It uses newline-delimited JSON-RPC over standard input and output:

```text
llmc++  -- JSON-RPC -->  agent: llm/generate
agent   -- MCP ------->  llmc++: initialize, tools/list, tools/call
agent   -- JSON-RPC -->  llmc++: generation result
```

`llm/generate` should carry the generation task, limits, resolved system
prompt, requested model when one was selected, and a protocol version with
optional capabilities. The agent calls the compiler's MCP tools, especially
`get_task`, `try_compile`, and `submit`, then returns its status and model id.

This lets users write an agent in Python or another language without copying
or modifying `llmcpp-agent`. The bundled Python program remains a reference
implementation and a convenient adapter, not a required intermediary.

The existing launch interface is the first transport:

```sh
llmc++ -fllm-agent='./my-agent' main.cpp
```

Later, add a distinct connection option for persistent agents:

```text
-fllm-agent-url=unix:///tmp/llmcpp-agent.sock
-fllm-agent-url=tcp://127.0.0.1:PORT
```

Keep command launching and socket connections separate. Standard input and
output give a launched agent a clear lifetime and straightforward sandboxing.
A network transport needs authentication, session handling, and concurrency
rules before it is suitable beyond local use.

### 13.3 Models and adapters

Keep provider-specific protocol translation in agents. An open-weight setup
should look like this:

```text
llmc++ -> versioned agent/MCP protocol -> adapter -> Mistral, vLLM, Ollama,
                                                     llama.cpp, or another server
```

This is preferable to a native C++ client for every provider. Servers that
claim OpenAI compatibility do not necessarily implement the same Responses
API or function-calling behavior as OpenAI. An adapter can translate its
server's chat and tool-call format into the stable llmcpp agent protocol.

A bundled generic OpenAI-chat-compatible adapter may be useful later, with a
base URL, model, credentials, and tool-call format supplied in its own
configuration. It must be optional: compatibility claims alone are not enough
to promise working compiler tools.

### 13.4 Useful controls

- `-fllm-model=<id>` requests a model from an agent; environment variables are
  defaults, not the only configuration interface.
- `-fllm-agent-config=<file>` passes agent-specific configuration without
  growing provider-specific compiler flags.
- Record redacted agent transcripts for diagnosis and replay.
- Print the prompt digest and agent identity in verbose output, and retain
  both in cache metadata.
- Ship a small reference Python package or example that implements the public
  protocol.

### 13.5 Scope discipline

`LLMCPP_BACKEND=auto` is convenient for exploration but weak for reproducible
builds: ambient API keys and installed CLIs can change the selected backend.
Prefer explicit agents or backends in CI.

The native OpenAI and Anthropic clients are useful convenience paths, but
should not become the extension mechanism for every model provider. Likewise,
provider-specific compiler flags should stay out of the driver and belong to
the selected agent.

---

## 14. Per-target generation options

Implemented: function-like modifiers, target policy precedence, agent-visible
effective settings, context-aware cache identity, metadata validation, and
unique temporary cache files. Bare `__llm__` remains valid; parentheses are
optional and carry extra options.

Some generation decisions belong to one `__llm__` target, rather than to the
whole compiler invocation. A target should be able to override command-line
defaults for cache use, model preference, and generation limits.

### 14.1 Syntax and compatibility

The long-term syntax should attach options directly to the modifier:

```cpp
__llm__(model("claude-opus-5-5"), no_cache)
void summarize(std::string_view text) {
    Write a one-sentence summary.
}
```

Use bare `__llm__` when no options are needed. `__llm__()` is also accepted.
Parentheses attach a small, declarative option language to the modifier.

Keep the object-like macro used to discover and erase bare modifiers. In the
coordinate-preserving parsing view, blank optional arguments before Clang sees
them. Parse the options from the original source and remove the complete
modifier from shadow compilations and rewritten output. This supports both
spellings without changing existing sources.

An alternative compatibility syntax is an adjacent C++ attribute, but it is
less cohesive and would require careful treatment of unknown-attribute
diagnostics. Prefer options attached directly to the modifier.

### 14.2 Option set and precedence

Start with a small set:

```cpp
__llm__(no_cache)
__llm__(cache("stable-v1"))
__llm__(model("claude-opus-5-5"))
__llm__(max_attempts(2), timeout(120))
```

Settings resolve from lowest to highest precedence:

```text
built-in defaults < command-line defaults < per-target __llm__ options
```

`no_cache` disables both reading and writing for that target. `cache("name")`
is a cache-key salt, not a filename; it gives a project an explicit way to
keep separate reviewed bodies for intentionally different policies.

`model("id")` is a requested model, carried in the public `llm/generate`
request described in §13. Agents that cannot supply it must report that fact,
rather than silently selecting another model. Model selection stays
provider-independent in the compiler and is implemented by the selected
agent.

### 14.3 Cache identity

Per-target cache controls must be part of the cache identity. Include the
normalized options, resolved system-prompt digest, compiler cache format, and
a compilation-context fingerprint in the entry's full key and metadata.

The initial context fingerprint should favor safe invalidation: hash relevant
compiler language options and macro definitions, the main source outside
generated prompt bodies, and the contents of transitive project headers. This
may regenerate after an unrelated header edit, but it avoids reusing a body
generated against stale visible C++ context. Later work can narrow the
fingerprint to declarations made available to the agent.

Cache reads should validate the entry schema version, full key, and context
fingerprint. Cache writes need unique temporary paths so simultaneous compiler
processes cannot race on one fixed `.tmp` file.
