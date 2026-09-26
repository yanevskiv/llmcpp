# How to develop llmc++

Install the repository hooks once:

```sh
pre-commit install
```

Run formatting, naming, and structural style checks with:

```sh
pre-commit run --all-files
```

The hooks run clang-format, clang-tidy naming checks, and the repository's
structural style checker. The complete contract is in [STYLE.md](../STYLE.md).

For the prototype design and planned milestones, read [PLAN.md](../PLAN.md).
