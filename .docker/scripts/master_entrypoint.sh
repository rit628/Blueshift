#!/bin/bash
if [[ -z $DEPLOY_NAMESPACE ]]; then
    ./${MASTER_PROGRAM_NAME} $@
else # debug mode, stop asap
    bash -c "exec -a master ./${MASTER_PROGRAM_NAME} $@ &"
    pkill -SIGSTOP -x master
    exec -a keepalive sleep infinity
fi