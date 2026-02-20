ARG BUILD_IMAGE_NAME=bls-builder
ARG RPI_PLATFORM=arm64

FROM --platform=${RPI_PLATFORM} dtcooper/raspberrypi-os:bookworm AS sysroot

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
--mount=type=cache,target=/var/lib/apt,sharing=locked \
apt-get update && apt-get install -y --no-install-recommends build-essential pigpio

FROM ${BUILD_IMAGE_NAME}:linux64
COPY --from=sysroot / /rpi64