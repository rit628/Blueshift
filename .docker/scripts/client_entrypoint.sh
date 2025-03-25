#!/bin/bash
# change hostname to container name for access in client binary
hostname $(host $(hostname -i) | awk '{print $NF}' | awk -F. '{print $1}')
if [[ -z $DEPLOY_NAMESPACE ]]; then
    ./${CLIENT_PROGRAM_NAME} $@
else  # debug mode, stop asap
    # change process name for unique identification with pgrep
    bash -c "exec -a $(hostname) ./${CLIENT_PROGRAM_NAME} $@ &"
    pkill -SIGSTOP -fx $(hostname)
    exec -a keepalive sleep infinity
fi