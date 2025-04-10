FROM debian:12

ARG CONTAINER_UID

ARG CONTAINER_GID

ARG WORKDIR

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install --no-install-recommends -y cmake build-essential python3 python3-venv ca-certificates lsb-release wget software-properties-common gnupg gdbserver && \
echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-19 main" | tee /etc/apt/sources.list.d/llvm.list && \
echo "deb-src http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-19 main" | tee -a /etc/apt/sources.list.d/llvm.list && \
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/llvm.asc && \
apt-get update && \
apt-get install -y clang-19 lldb-19 lld-19 && \
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-19 100 && \
update-alternatives --install /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-19 100 && \
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-19 100 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100

ENTRYPOINT [ "./bls" ]