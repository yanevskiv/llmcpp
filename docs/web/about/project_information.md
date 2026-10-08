# Project Information

`llmc++` is an experimental Clang-based compiler built with C++20. It and the bundled
`llmcpp-agent` have independent versions; both are currently at 1.0.0.

## Tools and platforms

The current dependency layout targets Debian or Ubuntu on x86-64 and LLVM/Clang
19. CMake builds the driver and its tests. Python supplies the external-agent
adapter, while the OpenAI and Anthropic API clients run inside the driver.
Docker provides a development environment with the same build entry point.

## License and development

`llmc++` is distributed under the GNU General Public License, version 3 or later.
The repository's `LICENSE.md` contains the license text. Third-party components
retain their own licenses.

Tests use deterministic mock agents so development does not require a model
subscription. The repository's `STYLE.md` defines the C++ style contract.
