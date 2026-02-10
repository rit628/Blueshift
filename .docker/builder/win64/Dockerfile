ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:base

RUN apt-get update && apt-get install -y \
mingw-w64 g++-mingw-w64-x86-64 gcc-mingw-w64-x86-64 g++-mingw-w64-ucrt64 gcc-mingw-w64-ucrt64 && \
ln -s /usr /win64