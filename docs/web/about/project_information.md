# Project Information

The project is named **llmcpp**; its compiler executable is **llmc++**. It is
an experimental Clang-based project built with C++20, currently at version 0.1.

## Tools and platforms

The current dependency layout targets Debian or Ubuntu on x86-64 and LLVM/Clang
19. CMake builds the driver and its tests. Python supplies the external-agent
adapter, while the OpenAI and Anthropic API clients run inside the driver.
Docker provides a development environment with the same build entry point.

## License and development

LLMCPP is distributed under the GNU Lesser General Public License, version 3.
The repository's `LICENSE.md` contains the license text. Third-party components
retain their own licenses.

Tests use deterministic mock agents so development does not require a model
subscription. The repository's `STYLE.md` defines the C++ style contract.
