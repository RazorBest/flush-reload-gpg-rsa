#!/bin/sh

set -o errexit
set -o xtrace
set -o nounset

GPG=./build_gpg/gnupg-1.4.12/g10/gpg #$HOME/gnupg-1.4.12/bin/gpg
ADDR=annotated_vm_v2.txt #annotated_vm_v1.txt #docs/addr/osx.txt
MESSAGE=message.txt
OUT=out.txt
CYCLES=$1

bin/probe "${GPG}" ${ADDR} ${OUT} "${CYCLES}" &
PROBE_PID=$!

sleep 0.01
(echo "GPG start"; ${GPG} --yes --sign ${MESSAGE}; echo "GPG end") &
GPG_PID=$!

trap "echo 'Received signal'; kill -TERM ${PROBE_PID} ${GPG_PID}" \
    INT QUIT

wait ${PROBE_PID}
wait ${GPG_PID}

python3 ./src/graph.py &
GRAPH_PID=$!

trap "echo 'Killing python'; kill -TERM ${GRAPH_PID}" INT QUIT
wait ${GRAPH_PID}
