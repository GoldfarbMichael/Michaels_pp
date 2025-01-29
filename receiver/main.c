#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <mastik/util.h>
#include <mastik/low.h>
#include <mastik/l3.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "prepare_receiver.h"
#include "training.h"
#include <limits.h>
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>



#define REPS 10
#define MESSAGE_SIZE 10
#define OFFSET_MONITOREDHEAD 0
#define OFFSET_NMONITORED 8
#define MISS_THRESHOLD 8
#define LOWER_CPU 0
#define UPPER_CPU 0
#define CLOCK_NORMALIZER 1
#define SYNC_FILE_R "/tmp/rceiver_prepared"
#define SYNC_FILE_S "/tmp/sender_prepared"
#define RECEIVER_LOG "../../cmake-build-debug/PrimeProbe/receiver_log.log"
#define SLOT INT_MAX
#define PROBE_CYCLES (2600000000/10000)  // should be a second


// #define SEM_DONE "/sem_done"
#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"

void wait_for_sender_ready(const char *file) {
    printf("Receiver: Waiting for Sender to be ready...\n");
    while (access(file, F_OK) == -1) {
        usleep(1); // Check every
    }
    printf("Receiver: Sender is ready. Proceeding...\n");
}


void signal_sender_ready(const char *file) {
    printf("Receiver: Signaling sender readiness...\n");
    FILE *sync_file = fopen(file, "w");
    if (sync_file == NULL) {
        perror("Failed to create sync file");
        exit(EXIT_FAILURE);
    }
    fclose(sync_file);
    printf("Receiver: Sender has been signaled.\n");
}

int countBitDifferences(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "r");
    FILE *fp2 = fopen(file2, "r");

    if (!fp1 || !fp2) {
        perror("Failed to open files");
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return -1;
    }

    int bit1, bit2;
    int differences = 0;
    int position = 0; // Track bit position
    int slice = 0; // Track slice number

    // Compare bits character by character
    while ((bit1 = fgetc(fp1)) != EOF && (bit2 = fgetc(fp2)) != EOF) {
        if (bit1 != bit2) {
            differences++;
        }
        position++;
        if (position % 1024 == 0) {
            printf("Slice %d: %d differing bits\n", slice, differences);
            slice++;
            differences = 0;
        }
    }

    // Check if files have different lengths
    if (bit1 != EOF || bit2 != EOF) {
        printf("Files have different lengths; comparison stops at position %d.\n", position);
    }

    fclose(fp1);
    fclose(fp2);
    return differences;
}

void restore_message(const uint16_t *res, uint16_t *message, int numOfSlices) {
    // check for nulls
    if (!res || !message) {
        fprintf(stderr, "Error: NULL pointer passed to restore_message.\n");
        return;
    }
    // printf("%4d ", (int16_t)(res[index]));
    for (int i = 0; i < MESSAGE_SIZE * numOfSlices; i++) {
        if ((int16_t)res[i] >= MISS_THRESHOLD) {
            message[i] = 1;
        }
        else if ((int16_t)res[i] == -1) {
         message[i] = -1;
        }
        else {
            message[i] = 0;
        }
    }
}

void stream_message_to_file(const uint16_t *message, const int numOfSlices) {
    //check for nulls
    if (!message) {
        fprintf(stderr, "Error: NULL pointer passed to steam_message_to_file.\n");
        return;
    }

    FILE *file = fopen("recoveredMessage.txt", "w");
    if (!file) {
        perror("Failed to open output file");
        return;
    }
    // Write bits to the file
    int index = 0;
    for (int sliceNum = 0; sliceNum < numOfSlices; sliceNum++) {
        fprintf(file, "Slice number: %d\n", sliceNum);
        for (int i = 0; i < MESSAGE_SIZE; i++) {
            fprintf(file, "%d", (int16_t)(message[index])); // Write the number in the cell
            index++;

        }
        fprintf(file,"\n"); // New line for the next slice
    }
    fclose(file);
}

void print_res(const uint16_t *res, const int numOfSlices) {
    // Write the repetition number to the file
    int index = 0;
    for (int sliceNum = 0; sliceNum < numOfSlices; sliceNum++) {
        printf("Slice number: %d\n ", sliceNum);
        for (int i = 0; i < MESSAGE_SIZE; i++) {
            printf("%4d ", (int16_t)(res[index])); // Write the number in the cell
            index++;

        }
        printf("\n"); // New line for the next slice
    }
}

uint64_t get_probe_time(l3pp_t l3, uint16_t *res) {
    uint64_t start = rdtscp64();
    l3_probe(l3, res);
    return rdtscp64() - start;
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


void log_time(const char *filename, const char *event, uint64_t time) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s %lu\n", event, time);
    fclose(file);
}



int main(int ac, char **av) {


    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    if (sem_turn_sender == SEM_FAILED || sem_turn_receiver == SEM_FAILED) {
        perror("sem_open failed");
        exit(1);
    }

    //start of preparation
    l3pp_t l3;
    set_cpu_range(LOWER_CPU, UPPER_CPU);

    prepare_receiver(&l3);
    const int numOfSlices = l3_getSlices(l3);
    signal_sender_ready(SYNC_FILE_R);


    uint16_t* res = (uint16_t*) calloc(MESSAGE_SIZE * numOfSlices, sizeof(uint16_t));
    for (int i = 0; i < MESSAGE_SIZE  * numOfSlices; i+= 2048/sizeof(uint16_t)) {
        res[i] = 1;
    }
    //end of preparation

    // uint64_t tProbeBefore = get_probe_time(l3,res);
    // printf("\nTime to probe beforee: %ld \n", tProbeBefore);

    FILE *file = fopen(RECEIVER_LOG, "w"); //empty log file
    if (!file) {
        perror("Failed to open log file");
        l3_unmonitorall(l3);
        l3_release(l3);
        free(res);
        return 1;
    }
    fclose(file);

    l3_unmonitorall(l3);
    for (int i = 0; i < numOfSlices; i++) {
        l3_monitor(l3, SET_INDEX + i * 1024);
    }

    uint16_t *tempRes = (uint16_t*) calloc(numOfSlices, sizeof(uint16_t));
    printf("\n--------starting probe--------\n");
    uint64_t end = 0;
    wait_for_sender_ready(SYNC_FILE_S);

    find_most_silent_set(&l3); // --------------------notice--------------

    for (int j = 0; j < MESSAGE_SIZE; j++) {

        // l3_unmonitorall(l3);
        // for (int i = 0; i < numOfSlices; i++) {
        //     l3_monitor(l3, SET_INDEX + i * 1024);
        // }

        sem_wait(sem_turn_receiver);
        uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
        l3_probecount(l3, tempRes);
        end = rdtscp64()/CLOCK_NORMALIZER;
        sem_post(sem_turn_sender);
        for (int slice = 0; slice < numOfSlices; slice++)
            res[slice * MESSAGE_SIZE + j] = tempRes[slice];

        log_time(RECEIVER_LOG, "Probing start", start);
        log_time(RECEIVER_LOG, "Probing end", end);
        log_time(RECEIVER_LOG, "Probing took", end - start);
        log_time(RECEIVER_LOG, "------------------------------------", 0);


        while (rdtscp64() < start + PROBE_CYCLES) {} //make the probe last for a second

    }

    // l3_repeatedprobecount(l3, MESSAGE_SIZE, res, INT_MAX);
    printf("--------probe ended--------\n\n");


    // log_time(RECEIVER_LOG, "Probing took", end - start);
    print_res(res, numOfSlices);
    // uint64_t tProbeAfter = get_probe_time(l3,res);
    // printf("\nTime to probe after: %ld \n", tProbeAfter);


    uint16_t *message = (uint16_t*) calloc(MESSAGE_SIZE * numOfSlices, sizeof(uint16_t));
    restore_message(res, message, numOfSlices);
    stream_message_to_file(message, numOfSlices);

    // int cpu = sched_getcpu();
    // if (cpu == -1) {
    //     perror("sched_getcpu");
    //     return 1;
    // }
    // printf("RECRIVER!!!!!!! process is running on CPU core: %d\n", cpu);

    // unlink(SYNC_FILE_S);
    // unlink(SYNC_FILE_R);
    l3_unmonitorall(l3);
    l3_release(l3);
    free(res);
    free(message);
    free(tempRes);




    return 0;
}