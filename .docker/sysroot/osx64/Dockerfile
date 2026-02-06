FROM debian:13

RUN apt-get update && apt-get install -y \
    build-essential \
    clang lldb lld llvm \
    cmake \
    git \
    python3 \
    libssl-dev \
    libxml2-dev \
    libz-dev \
    xz-utils \
    bzip2 \
    cpio \
    pkg-config \
    ca-certificates \
    curl

WORKDIR /build

RUN git clone https://github.com/tpoechtrager/osxcross.git
WORKDIR /build/osxcross

RUN curl -sSL https://github.com/joseluisq/macosx-sdks/releases/download/26.1/MacOSX26.1.sdk.tar.xz -o tarballs/MacOSX26.1.sdk.tar.xz

RUN OSX_VERSION_MIN=11 UNATTENDED=1 TARGET_DIR=/osxcross ./build.sh

RUN ln -s /osxcross ./target

RUN BRANCH=release/19.x ENABLE_COMPILER_RT_INSTALL=1 BUILD_DIR=/osxcross ./build_compiler_rt.sh