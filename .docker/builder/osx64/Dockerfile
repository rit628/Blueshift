ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:base

WORKDIR /tmp

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
--mount=type=cache,target=/var/lib/apt,sharing=locked \
apt-get update && apt-get install --no-install-recommends -y \
libxml2-dev libz-dev bzip2 cpio ca-certificates && \
git clone https://github.com/tpoechtrager/osxcross.git && \
cd osxcross && \
wget -O tarballs/MacOSX26.1.sdk.tar.xz https://github.com/joseluisq/macosx-sdks/releases/download/26.1/MacOSX26.1.sdk.tar.xz && \
OSX_VERSION_MIN=11 UNATTENDED=1 TARGET_DIR=/osx64 ./build.sh && \
ln -s /osx64 ./target && \
ENABLE_COMPILER_RT_INSTALL=1 ./build_compiler_rt.sh && \
rm -rf ../osxcross

ARG WORKDIR

WORKDIR ${WORKDIR}
