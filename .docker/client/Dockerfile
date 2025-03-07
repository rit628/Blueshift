FROM debian:12

ARG WORKDIR

ENV ENTR_INOTIFY_WORKAROUND=1

WORKDIR ${WORKDIR}

COPY .docker/scripts/client_entrypoint.sh .

RUN apt-get update && \
apt-get install -y entr host && \
chmod -R +x ./

ENTRYPOINT [ "./client_entrypoint.sh" ]