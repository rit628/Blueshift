FROM debian:12

ARG WORKDIR

RUN apt-get update && apt-get install -y procps

WORKDIR ${WORKDIR}

COPY .docker/scripts/master_entrypoint.sh .

ENTRYPOINT [ "./master_entrypoint.sh" ]