FROM debian:12

ARG WORKDIR

ENV ENTR_INOTIFY_WORKAROUND=1

WORKDIR ${WORKDIR}

RUN apt-get update && \
apt-get install -y entr && \
chmod -R +x ./

CMD ls ${MASTER_PROGRAM_NAME} | entr -nr ./${MASTER_PROGRAM_NAME}