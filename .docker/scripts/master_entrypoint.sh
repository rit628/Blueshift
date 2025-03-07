#!/bin/bash
ls ${MASTER_PROGRAM_NAME} | entr -nr ./${MASTER_PROGRAM_NAME} $@