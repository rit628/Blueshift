#!/bin/bash
hostname $(host $(hostname -i) | awk '{print $NF}' | awk -F. '{print $1}')
ls ${CLIENT_PROGRAM_NAME} | entr -nr ./${CLIENT_PROGRAM_NAME} $@