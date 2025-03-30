#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include "shared.h"
#define MESSAGE_SIZE 512


void log_time(const char *filename, const char *event, uint64_t time) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s: %lu\n", event, time);
    fclose(file);
}

void log_start_time(const char *filename, uint64_t time) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%lu,", time);
    fclose(file);
}



void print_string_from_bits(const uint16_t *bit_array) {
    if (MESSAGE_SIZE % 8 != 0) {
        fprintf(stderr, "Error: bit length must be a multiple of 8.\n");
        return;
    }
    for (size_t i = 0; i < MESSAGE_SIZE; i += 8) {
        char c = 0;
        for (int b = 0; b < 8; b++) {
            c = (c << 1) | (bit_array[i + b] & 1);
        }
        putchar(c);  // Print the character directly
    }
    putchar('\n');
}

void set_cpu_range(int start_cpu, int end_cpu) {
    cpu_set_t set;
    CPU_ZERO(&set); // Clear the CPU mask

    // Add CPUs in the specified range to the mask
    for (int i = start_cpu; i <= end_cpu; i++) {
        CPU_SET(i, &set);
    }

    // Apply the CPU affinity to the current process
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) != 0) {
        perror("sched_setaffinity");
    }
}

void print_core(){
    int core = sched_getcpu();
    if (core != -1) {
        printf("Running on core: %d\n", core);
    } else {
        perror("sched_getcpu");
    }
}




