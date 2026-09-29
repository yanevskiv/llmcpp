FROM ubuntu:latest

# Install build tools, dependency download tools, and development checks.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-19 \
        clang-format-19 \
        clang-tidy-19 \
        cmake \
        curl \
        doxygen \
        texlive-latex-extra \
        texlive-fonts-recommended \
        git \
        libllvm19 \
        ninja-build \
        pre-commit \
        python3 \
        python3-pip \
        python3-venv \
    && ln -s /usr/bin/clang-format-19 /usr/local/bin/clang-format \
    && ln -s /usr/bin/clang-tidy-19 /usr/local/bin/clang-tidy

# Seed the dependency volume using the project's dependency versions.
WORKDIR /workspace
COPY do_fetch_deps.sh /usr/local/bin/llmcpp-fetch-deps
RUN LLMCPP_DEPS_DIR=/workspace/deps bash /usr/local/bin/llmcpp-fetch-deps \
    && rm -rf /var/lib/apt/lists/*

COPY docker/entrypoint.sh /usr/local/bin/llmcpp-entrypoint
RUN chmod +x /usr/local/bin/llmcpp-entrypoint

ENTRYPOINT ["/usr/local/bin/llmcpp-entrypoint"]
CMD ["./do_build.sh"]
