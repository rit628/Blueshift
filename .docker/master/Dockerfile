FROM debian:12

ARG WORKDIR

WORKDIR ${WORKDIR}

COPY .docker/scripts/master_entrypoint.sh .

ENTRYPOINT [ "./master_entrypoint.sh" ]