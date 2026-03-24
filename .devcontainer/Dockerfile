FROM mcr.microsoft.com/devcontainers/base:ubuntu

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        gdb \
        make \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        check \
        valgrind \
    && rm -rf /var/lib/apt/lists/*
