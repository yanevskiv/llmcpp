#!/usr/bin/env python3
"""Checks llmcpp structural style rules that clang-format cannot express."""

from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_ROOTS = (ROOT / "include", ROOT / "src")
EXTRA_SOURCES = (ROOT / "test" / "integration_tests.cpp",)
SNAKE_CASE = re.compile(r"^[a-z][a-z0-9]*(?:_[a-z0-9]+)*\.(?:cpp|h)$")
DOXYGEN_BLOCK = re.compile(r"(?m)^(?P<indent> *)/\*\*")
FIELD_DECLARATION = re.compile(
    r"(?m)^(?P<indent> +)(?![^\n]*\breturn\b)[^\n;]*\bm_[a-z0-9_]+\b[^\n;]*;"
)


def sources() -> list[pathlib.Path]:
    """Returns project-owned C++ sources governed by the style contract."""
    result: list[pathlib.Path] = []
    for directory in SOURCE_ROOTS:
        result.extend(directory.rglob("*.cpp"))
        result.extend(directory.rglob("*.h"))
    result.extend(path for path in EXTRA_SOURCES if path.exists())
    return sorted(result)


def comment_positions(text: str) -> list[tuple[int, str]]:
    """Returns comment offsets while ignoring comment markers in literals."""
    comments: list[tuple[int, str]] = []
    index = 0
    quote = ""
    while index < len(text):
        if quote:
            if text[index] == "\\":
                index += 2
                continue
            if text[index] == quote or text[index] in "\r\n":
                quote = ""
            index += 1
            continue
        if text[index] in "\"'":
            quote = text[index]
            index += 1
            continue
        if text.startswith("//", index):
            end = text.find("\n", index)
            end = len(text) if end < 0 else end
            comments.append((index, text[index:end]))
            index = end
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = len(text) if end < 0 else end + 2
            comments.append((index, text[index:end]))
            index = end
            continue
        index += 1
    return comments


def callable_ranges(text: str) -> list[tuple[int, int]]:
    """Finds callable bodies with a conservative brace and signature scan."""
    masked = list(text)
    for offset, comment in comment_positions(text):
        for index in range(offset, offset + len(comment)):
            if masked[index] not in "\r\n":
                masked[index] = " "
    clean = "".join(masked)
    raw_pattern = re.compile(r'(?:u8|u|U|L)?R"([^ ()\\\t\r\n]{0,16})\([\s\S]*?\)\1"')
    for match in raw_pattern.finditer(clean):
        for index in range(match.start(), match.end()):
            if masked[index] not in "\r\n":
                masked[index] = " "
    index = 0
    quote = ""
    while index < len(masked):
        if quote:
            if masked[index] == "\\":
                masked[index] = " "
                if index + 1 < len(masked):
                    masked[index + 1] = " "
                index += 2
                continue
            if masked[index] == quote or masked[index] in "\r\n":
                quote = ""
            if masked[index] not in "\r\n":
                masked[index] = " "
            index += 1
            continue
        if masked[index] in "\"'":
            quote = masked[index]
            masked[index] = " "
        index += 1
    clean = "".join(masked)
    stack: list[tuple[int, bool]] = []
    ranges: list[tuple[int, int]] = []
    for match in re.finditer(r"[{}]", clean):
        if match.group() == "{":
            prefix = clean[max(0, match.start() - 300) : match.start()]
            boundary = max(prefix.rfind(";"), prefix.rfind("}"), prefix.rfind("{"))
            signature = prefix[boundary + 1 :].strip()
            excluded = re.match(r"^(?:namespace|class|struct|enum|union)\b", signature)
            callable_body = not excluded and (")" in signature or re.search(r"\]\s*(?:\([^)]*\))?\s*$", signature)
            )
            stack.append((match.start(), bool(callable_body) or any(flag for _, flag in stack)))
        elif stack:
            begin, callable_body = stack.pop()
            if callable_body:
                ranges.append((begin, match.end()))
    return ranges


def line_number(text: str, offset: int) -> int:
    """Returns the one-based line number for an offset."""
    return text.count("\n", 0, offset) + 1


def check(path: pathlib.Path) -> list[str]:
    """Returns style violations for one file."""
    relative = path.relative_to(ROOT)
    text = path.read_text()
    errors: list[str] = []
    if path.parent.name == "llmcpp" and not SNAKE_CASE.fullmatch(path.name):
        errors.append(f"{relative}: filename must use lowercase snake_case")
    if not text.startswith("/*"):
        errors.append(f"{relative}: file must start with a /* ... */ header")
    if "\t" in text:
        errors.append(f"{relative}: tabs are forbidden")
    if "///" in text:
        errors.append(f"{relative}: use /** ... */ Doxygen blocks, not ///")
    if re.search(r"}\s*//\s*namespace\b", text):
        errors.append(f"{relative}: namespace-closing comments are forbidden")
    if re.search(r"#endif\s*//", text):
        errors.append(f"{relative}: header-guard closing comments are forbidden")

    if path.suffix == ".h":
        for match in DOXYGEN_BLOCK.finditer(text):
            prefix = text[: match.start()]
            preceding_lines = prefix.splitlines()
            blank_count = 0
            while preceding_lines and not preceding_lines[-1].strip():
                blank_count += 1
                preceding_lines.pop()
            if blank_count:
                errors.append(
                    f"{relative}:{line_number(text, match.start())}: "
                    "Doxygen documentation must not have a preceding blank line"
                )

        for match in FIELD_DECLARATION.finditer(text):
            preceding = text[: match.start()].rstrip()
            if not preceding.endswith("*/"):
                errors.append(
                    f"{relative}:{line_number(text, match.start())}: "
                    "fields must have immediately preceding Doxygen documentation"
                )

    ranges = callable_ranges(text)
    for offset, comment in comment_positions(text):
        if any(begin < offset < end for begin, end in ranges):
            if comment.strip() == "// Empty.":
                continue
            errors.append(
                f"{relative}:{line_number(text, offset)}: comments are forbidden in callable bodies"
            )
    return errors


def main() -> int:
    """Checks all governed sources and returns a process status."""
    errors = [error for path in sources() for error in check(path)]
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
