FROM debian:13-slim

ARG WORKDIR
ARG LLVM_VERSION
ARG CMAKE_VERSION

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR ${WORKDIR}

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
--mount=type=cache,target=/var/lib/apt,sharing=locked \
apt-get update && apt-get install --no-install-recommends -y \
build-essential make ccache python3 python3-venv git procps gdbserver wget \
libssl-dev libpsl-dev pkg-config && \
wget -qO- https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.0-rc2-linux-$(uname -m).tar.gz | tar --strip-components=1 -xz -C /usr/local && \
echo "deb http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${LLVM_VERSION} main" | tee /etc/apt/sources.list.d/llvm.list && \
echo "deb-src http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${LLVM_VERSION} main" | tee -a /etc/apt/sources.list.d/llvm.list && \
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/llvm.asc && \
apt-get update && apt-get install --no-install-recommends -y \
clang-${LLVM_VERSION} lldb-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} clangd-${LLVM_VERSION} && \
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100 && \
mkdir .venv && chmod 777 .venv && \
mkdir /ccache && chmod 777 /ccache && \
groupadd gpio