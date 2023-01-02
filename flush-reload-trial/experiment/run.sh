#!/bin/bash

python3 -m pip install matplotlib numpy Levenshtein

set -o xtrace

export GNUPGHOME=./dot-gnupg

(sleep 0.05 && ../gpg -d hello.txt.gpg) | ../src/spy mygpg_listen.conf l > out.txt

python3 graph.py out.txt
python3 graph.py out.txt --plot
