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
cd src
./spy ../experiment/mygpg.conf > out.txt
```

In another window, run (twice):
```
GNUPGHOME=./experiment/dot-gnupg/ ./gpg -d experiment/hello.txt.gpg
GNUPGHOME=./experiment/dot-gnupg/ ./gpg -d experiment/hello.txt.gpg
```

That's because the spy ignores the first run.

After running gpg the second time, the spy should stop. The output will
be in out.txt.

# Steps that I took in order to reproduce the experiment

## 1. Download and build GnuPG

I downloaded GnuPG 1.4.12 and built it statically with --std=gnu89. Also, I
made sure to build it with the source file version, to be easier to inspect
the symbols in the binary.
```
./configure CFLAGS="--std=gnu89" --disable-dependency-tracking --disable-asm --prefix="${PWD}"
make
make install
```

After this, you should have an elf binary called gpg. If you built it in the
folder with the source code, it should be under the g10 folder.

The way I built gpg was taken from here: https://github.com/hjdrahub/flush-reload

## 2. Generate a pair of keys and test gpg.

GnuPG needs a home directory to read/write the keys. If it is not specified,
it defaults in $HOME/.gnupg. You can specify it with the env variable
GNUPGHOME.

In our case, GNUPGHOME is ./experiment/dot-gnupg, which already has a pair
of keys. Those keys were used to obtain ./experiment/hello.txt.gpg. In
order to decrypt the file you can run:
```
GNUPGHOME=./experiment/dot-gnupg ./gpg -d ./experiment/hello.txt.gpg
```

Where, "./gpg" refers to the binary that was compiled eariler from source.

## 3. 

