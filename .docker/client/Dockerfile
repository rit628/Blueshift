FROM debian:12

ARG WORKDIR

ENV ENTR_INOTIFY_WORKAROUND=1

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install -y entr && \
chmod -R +x ./

CMD ls ${CLIENT_PROGRAM_NAME} | entr -nr ./${CLIENT_PROGRAM_NAME}