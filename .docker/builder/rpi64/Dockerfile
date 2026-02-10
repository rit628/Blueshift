ARG BUILD_IMAGE_NAME=bls-builder
ARG RPI_PLATFORM=arm64

FROM --platform=${RPI_PLATFORM} dtcooper/raspberrypi-os:bookworm AS sysroot

RUN apt-get update && apt-get install -y build-essential pigpio

FROM ${BUILD_IMAGE_NAME}:base
COPY --from=sysroot / /rpi64