FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    clang-tidy \
    cmake \
    cppcheck \
    curl \
    git \
    gdb \
    libclang-rt-18-dev \
    ninja-build \
    python3 \
    python3-pip \
    llvm \
    && rm -rf /var/lib/apt/lists/*

ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH=/root/.cargo/bin:${PATH}

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --profile minimal

WORKDIR /workspace

CMD ["/bin/bash"]
