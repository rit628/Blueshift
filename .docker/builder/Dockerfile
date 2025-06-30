FROM debian:12

ARG WORKDIR

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install --no-install-recommends -y build-essential python3 python3-venv ca-certificates lsb-release wget software-properties-common gnupg gdbserver \ 
libasound2-dev libpulse-dev libaudio-dev libjack-dev libsndio-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev && \
wget -qO- https://cmake.org/files/v3.30/cmake-3.30.0-rc2-linux-$(uname -m).tar.gz | tar --strip-components=1 -xz -C /usr/local && \
echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-19 main" | tee /etc/apt/sources.list.d/llvm.list && \
echo "deb-src http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-19 main" | tee -a /etc/apt/sources.list.d/llvm.list && \
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/llvm.asc && \
apt-get update && \
apt-get install -y clang-19 lldb-19 lld-19 llvm-19 && \
update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-19 100 && \
update-alternatives --install /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-19 100 && \
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-19 100 && \
update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-19 100 && \
update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-19 100 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100 && \
mkdir .venv && \
chmod 777 .venv && \
groupadd gpio

ENTRYPOINT [ "./bls" ]