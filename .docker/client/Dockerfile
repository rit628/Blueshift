FROM debian:12

ARG WORKDIR

RUN apt-get update && apt-get install -y host procps

WORKDIR ${WORKDIR}

COPY .docker/scripts/client_entrypoint.sh .

ENTRYPOINT [ "./client_entrypoint.sh" ]