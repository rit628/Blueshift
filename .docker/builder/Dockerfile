FROM debian:12

ARG WORKDIR

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install --no-install-recommends -y cmake build-essential python3 ca-certificates lsb-release wget software-properties-common gnupg gdbserver && \
echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-18 main" | tee /etc/apt/sources.list.d/llvm.list && \
echo "deb-src http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-18 main" | tee -a /etc/apt/sources.list.d/llvm.list && \
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/llvm.asc && \
apt-get update && \
apt-get install -y clang-18 lldb-18 lld-18 && \
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-18 100 && \
update-alternatives --install /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-18 100 && \
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-18 100 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100

ENTRYPOINT [ "./blueshift" ]