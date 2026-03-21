FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    clang-tidy \
    cmake \
    cppcheck \
    git \
    gdb \
    libclang-rt-18-dev \
    ninja-build \
    python3 \
    python3-pip \
    llvm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
