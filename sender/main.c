#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mastik/l3.h>
#include <mastik/impl.h>
#include "correlated_set.h"
#include <sched.h>
#define LOWER_CPU 4
#define UPPER_CPU 4
#define CLOCK_NORMALIZER 2600000
#define SYNC_FILE_R "/tmp/rceiver_prepared"
#define SYNC_FILE_S "/tmp/sender_prepared"

void wait_for_receiver_ready(const char *file) {
    printf("Sender: Waiting for receiver to be ready...\n");
    while (access(file, F_OK) == -1) {
        usleep(1000); // Check every
    }
    printf("Sender: Receiver is ready. Proceeding...\n");
}

void signal_receiver_ready(const char *file) {
    printf("Sender: Signaling Receiver readiness...\n");
    FILE *sync_file = fopen(file, "w");
    if (sync_file == NULL) {
        perror("Failed to create sync file");
        exit(EXIT_FAILURE);
    }
    fclose(sync_file);
    printf("Sender: Receiver has been signaled.\n");
}

void openLink(char* url) {
    char command[2048];
    snprintf(command, sizeof(command), "taskset -c 0 firefox %s &", url);
    // Execute the command
    int result = system(command);

    // Check if the command was executed successfully
    if (result != 0) {
        fprintf(stderr, "Failed to open the URL\n");
    }
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



int main(int argc, char *argv[]) {

    //start of sender preparation
    l3pp_t l3;
    uint8_t *message = calloc(MESSAGE_SIZE, sizeof(uint8_t));

    set_cpu_range(LOWER_CPU, UPPER_CPU);

    wait_for_receiver_ready(SYNC_FILE_R);
    prepare_sender(&l3, message);
    //end of sender preparation
    void* monitoredHead = getHead(l3, 0);
    uint64_t traverseTime = get_time_to_traverse(monitoredHead);
    // printf("Time to traverse: %ld \n", traverseTime);
    delayloop(3000000000U);
    printf("----------------started priming----------------\n");
    signal_receiver_ready(SYNC_FILE_S);
    uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
    correlated_prime(monitoredHead, message,INT_MAX, traverseTime);
    uint64_t end = rdtscp64()/CLOCK_NORMALIZER;
    printf("---------------- priming ended----------------\n");

    printf("Prime started at: %lu\nPrime ended at: %lu\n\n", start, end);

    free(message);
    l3_release(l3);
    return EXIT_SUCCESS;
}