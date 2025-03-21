#!/bin/bash
hostname $(host $(hostname -i) | awk '{print $NF}' | awk -F. '{print $1}')
# change process name for unique identification with pgrep
exec -a $(hostname) ./${CLIENT_PROGRAM_NAME} $@ 