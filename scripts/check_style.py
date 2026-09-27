#!/usr/bin/env python3
"""Enforce the structural parts of the llmcpp C++ style contract.

clang-format handles layout. This checker covers project structure and
documentation rules that a formatter cannot express.
"""

from __future__ import annotations

import pathlib
import re
import sys


# Project files covered by the style contract.
PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_DIRECTORIES = (PROJECT_ROOT / "include", PROJECT_ROOT / "src")
STANDALONE_SOURCES = (PROJECT_ROOT / "tests" / "llmcpp_tests.cpp",)

# C++ constructs used by individual checks.
SNAKE_CASE_FILENAME = re.compile(
    r"""
    ^[a-z][a-z0-9]*
    (?:_[a-z0-9]+)*
    \.(?:cpp|h)$
    """,
    re.VERBOSE,
)
DOXYGEN_BLOCK = re.compile(r"(?m)^ */\*\*")
FIELD_DECLARATION = re.compile(
    r"""
    ^[ ]+
    (?![^\n]*\breturn\b)
    [^\n;]*\bm_[a-z0-9_]+\b[^\n;]*;
    """,
    re.MULTILINE | re.VERBOSE,
)
RAW_STRING = re.compile(
    r"""
    (?:u8|u|U|L)?R"
    ([^ ()\\\t\r\n]{0,16})
    \([\s\S]*?\)\1"
    """,
    re.VERBOSE,
)
BRACE = re.compile(r"[{}]")
TYPE_DECLARATION = re.compile(r"^(?:namespace|class|struct|enum|union)\b")
LAMBDA_SIGNATURE = re.compile(r"\]\s*(?:\([^)]*\))?\s*$")


# Collect the C++ files governed by the style contract.
def project_sources() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for directory in SOURCE_DIRECTORIES:
        files.extend(directory.rglob("*.cpp"))
        files.extend(directory.rglob("*.h"))
    files.extend(path for path in STANDALONE_SOURCES if path.exists())
    return sorted(files)


# Find comments without mistaking quoted markers for comments.
def find_comments(text: str) -> list[tuple[int, str]]:
    comments: list[tuple[int, str]] = []
    position = 0
    quote = ""

    while position < len(text):
        character = text[position]

        if quote:
            if character == "\\":
                position += 2
                continue
            if character == quote or character in "\r\n":
                quote = ""
            position += 1
            continue

        if character in "\"'":
            quote = character
            position += 1
            continue

        if text.startswith("//", position):
            end = text.find("\n", position)
            end = len(text) if end == -1 else end
            comments.append((position, text[position:end]))
            position = end
            continue

        if text.startswith("/*", position):
            end = text.find("*/", position + 2)
            end = len(text) if end == -1 else end + 2
            comments.append((position, text[position:end]))
            position = end
            continue

        position += 1

    return comments


# Blank a range while preserving its line structure.
def blank_range(characters: list[str], start: int, end: int) -> None:
    for position in range(start, end):
        if characters[position] not in "\r\n":
            characters[position] = " "


# Hide comments from structure scans.
def mask_comments(characters: list[str], text: str) -> None:
    for offset, comment in find_comments(text):
        blank_range(characters, offset, offset + len(comment))


# Hide raw strings from structure scans.
def mask_raw_strings(characters: list[str]) -> None:
    for match in RAW_STRING.finditer("".join(characters)):
        blank_range(characters, match.start(), match.end())


# Hide ordinary strings and character literals from structure scans.
def mask_quoted_literals(characters: list[str]) -> None:
    position = 0
    quote = ""

    while position < len(characters):
        character = characters[position]

        if quote:
            if character == "\\":
                characters[position] = " "
                if position + 1 < len(characters):
                    characters[position + 1] = " "
                position += 2
                continue
            if character == quote or character in "\r\n":
                quote = ""
            if character not in "\r\n":
                characters[position] = " "
            position += 1
            continue

        if character in "\"'":
            quote = character
            characters[position] = " "
        position += 1


# Replace comments and literals with whitespace for structure scans.
def mask_non_code(text: str) -> str:
    characters = list(text)
    mask_comments(characters, text)
    mask_raw_strings(characters)
    mask_quoted_literals(characters)
    return "".join(characters)


# Extract the declaration fragment before an opening brace.
def signature_before(code: str, brace_offset: int) -> str:
    prefix = code[max(0, brace_offset - 300) : brace_offset]
    boundary = max(prefix.rfind(token) for token in ";}{")
    return prefix[boundary + 1 :].strip()


# Identify function, method, constructor, and lambda bodies.
def opens_callable(signature: str) -> bool:
    if TYPE_DECLARATION.match(signature):
        return False
    return ")" in signature or bool(LAMBDA_SIGNATURE.search(signature))


# Locate callable bodies with a conservative brace scan.
def callable_ranges(text: str) -> list[tuple[int, int]]:
    code = mask_non_code(text)
    stack: list[tuple[int, bool]] = []
    ranges: list[tuple[int, int]] = []

    for brace in BRACE.finditer(code):
        if brace.group() == "{":
            signature = signature_before(code, brace.start())
            nested_in_callable = any(is_callable for _, is_callable in stack)
            stack.append(
                (brace.start(), nested_in_callable or opens_callable(signature))
            )
            continue

        if stack:
            start, is_callable = stack.pop()
            if is_callable:
                ranges.append((start, brace.end()))

    return ranges


# Convert a text offset to a one-based line number.
def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


# Check that Doxygen blocks immediately follow the preceding declaration.
def check_doxygen_spacing(
    path: pathlib.Path, text: str, errors: list[str]
) -> None:
    relative_path = path.relative_to(PROJECT_ROOT)

    for match in DOXYGEN_BLOCK.finditer(text):
        preceding_lines = text[: match.start()].splitlines()
        blank_lines = 0
        while preceding_lines and not preceding_lines[-1].strip():
            blank_lines += 1
            preceding_lines.pop()

        if blank_lines:
            errors.append(
                f"{relative_path}:{line_number(text, match.start())}: "
                "Doxygen documentation must not have a preceding blank line"
            )


# Check that fields have immediately preceding Doxygen documentation.
def check_field_documentation(
    path: pathlib.Path, text: str, errors: list[str]
) -> None:
    relative_path = path.relative_to(PROJECT_ROOT)

    for match in FIELD_DECLARATION.finditer(text):
        preceding_text = text[: match.start()].rstrip()
        if not preceding_text.endswith("*/"):
            errors.append(
                f"{relative_path}:{line_number(text, match.start())}: "
                "fields must have immediately preceding Doxygen documentation"
            )


# Check that callable bodies contain no explanatory comments.
def check_body_comments(
    path: pathlib.Path, text: str, errors: list[str]
) -> None:
    relative_path = path.relative_to(PROJECT_ROOT)
    body_ranges = callable_ranges(text)

    for offset, comment in find_comments(text):
        inside_body = any(start < offset < end for start, end in body_ranges)
        if inside_body and comment.strip() != "// Empty.":
            errors.append(
                f"{relative_path}:{line_number(text, offset)}: "
                "comments are forbidden in callable bodies"
            )


# Check one C++ file against the structural style rules.
def check_file(path: pathlib.Path) -> list[str]:
    relative_path = path.relative_to(PROJECT_ROOT)
    text = path.read_text()
    errors: list[str] = []

    if path.parent.name == "llmcpp" and not SNAKE_CASE_FILENAME.fullmatch(
        path.name
    ):
        errors.append(f"{relative_path}: filename must use lowercase snake_case")
    if not text.startswith("/*"):
        errors.append(f"{relative_path}: file must start with a /* ... */ header")
    if "\t" in text:
        errors.append(f"{relative_path}: tabs are forbidden")
    if "///" in text:
        errors.append(f"{relative_path}: use /** ... */ Doxygen blocks, not ///")
    if re.search(r"}\s*//\s*namespace\b", text):
        errors.append(f"{relative_path}: namespace-closing comments are forbidden")
    if re.search(r"#endif\s*//", text):
        errors.append(
            f"{relative_path}: header-guard closing comments are forbidden"
        )

    if path.suffix == ".h":
        check_doxygen_spacing(path, text, errors)
        check_field_documentation(path, text, errors)
    check_body_comments(path, text, errors)
    return errors


# Check every governed source and report all violations together.
def main() -> int:
    errors = [
        error
        for path in project_sources()
        for error in check_file(path)
    ]
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
