ARG BUILD_IMAGE_NAME=bls-builder

FROM ${BUILD_IMAGE_NAME}:linux64

ARG EMSCRIPTEN_VERSION=4.0.0

ENV EM_LLVM_ROOT=/usr/lib/llvm-20/bin

WORKDIR /

RUN apt-get install -y git && \
git clone https://github.com/emscripten-core/emsdk.git && \
cd emsdk && \
./emsdk install ${EMSCRIPTEN_VERSION} && \
./emsdk activate ${EMSCRIPTEN_VERSION} && \
echo 'source "/emsdk/emsdk_env.sh"' >> $HOME/.bash_profile && \
ln -s /emsdk/upstream/emscripten /wasm32

ARG WORKDIR

WORKDIR ${WORKDIR}