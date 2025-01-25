FROM debian:12

ARG WORKDIR

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install --no-install-recommends -y cmake build-essential python3 ca-certificates lsb-release wget software-properties-common gnupg && \
wget -qO- https://apt.llvm.org/llvm.sh | bash -s -- 18 && \
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100

ENTRYPOINT [ "./blueshift" ]