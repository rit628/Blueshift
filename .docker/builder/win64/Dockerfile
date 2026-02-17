ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:linux64

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
--mount=type=cache,target=/var/lib/apt,sharing=locked \
apt-get update && apt-get install -y --no-install-recommends \
mingw-w64 g++-mingw-w64-x86-64 gcc-mingw-w64-x86-64 g++-mingw-w64-ucrt64 gcc-mingw-w64-ucrt64 && \
ln -s /usr /win64