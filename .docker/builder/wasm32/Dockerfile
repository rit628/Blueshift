ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:linux64

ARG EMSCRIPTEN_VERSION=4.0.11

ARG LLVM_VERSION

ENV EM_LLVM_ROOT=/usr/lib/llvm-${LLVM_VERSION}/bin

WORKDIR /

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
--mount=type=cache,target=/var/lib/apt,sharing=locked \
apt-get install -y --no-install-recommends git && \
git clone https://github.com/emscripten-core/emsdk.git && \
cd emsdk && \
./emsdk install ${EMSCRIPTEN_VERSION} && \
./emsdk activate ${EMSCRIPTEN_VERSION} && \
echo 'source "/emsdk/emsdk_env.sh"' >> $HOME/.bash_profile && \
ln -s /emsdk/upstream/emscripten /wasm32

ARG WORKDIR

WORKDIR ${WORKDIR}