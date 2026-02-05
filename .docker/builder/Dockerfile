FROM debian:13

ARG WORKDIR
ARG LLVM_VERSION=20
ARG CMAKE_VERSION=3.30

WORKDIR ${WORKDIR}

# TODO: Remove SHA1 deprecation workaround after llvm fixes their repository

RUN apt-get update && \
apt-get install --no-install-recommends -y build-essential procps python3 python3-venv ca-certificates lsb-release wget gnupg gdbserver \ 
libssl-dev libpsl-dev mingw-w64 g++-mingw-w64-x86-64 gcc-mingw-w64-x86-64 g++-mingw-w64-ucrt64 gcc-mingw-w64-ucrt64 \
libasound2-dev libpulse-dev libaudio-dev libjack-dev libsndio-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev && \
wget -qO- https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.0-rc2-linux-$(uname -m).tar.gz | tar --strip-components=1 -xz -C /usr/local && \
mkdir -p /etc/crypto-policies/back-ends/ && \
cp /usr/share/apt/default-sequoia.config /etc/crypto-policies/back-ends/apt-sequoia.config && \
sed -i 's/2026/2027/g' /etc/crypto-policies/back-ends/apt-sequoia.config && \
echo "deb http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${LLVM_VERSION} main" | tee /etc/apt/sources.list.d/llvm.list && \
echo "deb-src http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${LLVM_VERSION} main" | tee -a /etc/apt/sources.list.d/llvm.list && \
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/llvm.asc && \
apt-get update && \
apt-get install -y clang-${LLVM_VERSION} lldb-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} && \
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 && \
mkdir .venv && \
chmod 777 .venv && \
groupadd gpio

ENTRYPOINT [ "./bls" ]