# Flush-Reload on GnuPG 1.4.12

A side channel attack that replicates the work of Yuval Yarom and Katrina Falkner:
FLUSH+RELOAD: A High Resolution, Low Noise, L3 Cache Side-Channel Attack.

## How to build the spy
```
cd src
make
```

## How to run

You need to run the victim and the spy as 2 separate processes. The victim
is the vulnerable gpg elf binary.

In one window, run:
```
./src/spy experiment/mygpg.conf > out.txt
```

In another window, run (twice):
```
GNUPGHOME=./experiment/dot_gnupg/ ./gpg -d experiment/hello.txt.gpg
GNUPGHOME=./experiment/dot_gnupg/ ./gpg -d experiment/hello.txt.gpg
```

That's because the spy ignores the first run.

After running gpg the second time, the spy should stop. The output will
be in out.txt.
