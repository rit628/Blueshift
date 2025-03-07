FROM debian:12

ARG WORKDIR

ENV ENTR_INOTIFY_WORKAROUND=1

WORKDIR ${WORKDIR}

COPY .docker/scripts/master_entrypoint.sh .

RUN apt-get update && \
apt-get install -y entr && \
chmod -R +x ./

ENTRYPOINT [ "./master_entrypoint.sh" ]