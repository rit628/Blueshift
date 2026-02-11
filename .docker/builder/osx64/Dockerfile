ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:linux64

WORKDIR /tmp

RUN apt-get update && \
apt-get install --no-install-recommends -y git libxml2-dev libz-dev xz-utils bzip2 cpio pkg-config curl && \
git clone https://github.com/tpoechtrager/osxcross.git && \
cd osxcross && \
curl -sSL https://github.com/joseluisq/macosx-sdks/releases/download/26.1/MacOSX26.1.sdk.tar.xz -o tarballs/MacOSX26.1.sdk.tar.xz && \
OSX_VERSION_MIN=11 UNATTENDED=1 TARGET_DIR=/osx64 ./build.sh && \
ln -s /osx64 ./target && \
ENABLE_COMPILER_RT_INSTALL=1 ./build_compiler_rt.sh

ARG WORKDIR

WORKDIR ${WORKDIR}