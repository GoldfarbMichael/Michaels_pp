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
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

#define MISS_THRESHOLD 8
#define LOWER_CPU 0
#define UPPER_CPU 0
#define CLOCK_NORMALIZER (3600*1)
#define RECEIVER_LOG "../../cmake-build-debug/PrimeProbe/receiver_log.log"
#define PROBE_CYCLES (3600000000/CLOCK_NORMALIZER)  // should be a second

#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"



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

/**
 *
 * @param restoredMessage the restored message (size of the message is MESSAGE_SIZE * NumOfSlices)
 * @param NumOfSlices number of slices
 * @return returns true if the message is restored at with at least 70% accuracy
 **/
int is_restored(uint16_t *restoredMessage, int NumOfSlices)
{
    uint16_t message[MESSAGE_SIZE]; //init message
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        if (i%2 == 0)
            message[i] = 1;
        else
            message[i] = 0;
    }

    for (int sliceNum = 0; sliceNum < NumOfSlices; sliceNum++)
    {
        float sum = 0;
        for (int i = 0; i < MESSAGE_SIZE; i++)
        {
            if (restoredMessage[sliceNum*NUM_OF_SETS_IN_SLICE + i] == message[i])
                sum++;
            else
                sum --;
        }
        if (sum / (float)MESSAGE_SIZE >= 0.9)
        {
            printf("THE SUM IS %f\n", sum);
            //print the restored message
            for (int i = 0; i < MESSAGE_SIZE; i++)
            {
                printf("%d ", restoredMessage[sliceNum*NUM_OF_SETS_IN_SLICE + i]);
            }
            return 1;
        }
    }
    return 0;
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

void evict_monitor_and_evict_sets(l3pp_t l3, int numOfSlices) {
    uint16_t* res = (uint16_t*) calloc(numOfSlices * MESSAGE_SIZE, sizeof(uint16_t));
    for (int i = 0; i < numOfSlices; i++) {
        l3_monitor(l3, SET_INDEX + i * NUM_OF_SETS_IN_SLICE);
    }
    l3_repeatedprobe(l3, 20, res, 1);
    free(res);
}


int main(int ac, char **av) {

    sem_t *sem_mapping = sem_open(SEM_MAPPING, O_RDWR);
    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_SENDER, O_RDWR);
    if (sem_turn_sender == SEM_FAILED || sem_turn_receiver == SEM_FAILED || sem_mapping == SEM_FAILED) {
        perror("sem_open failed");
        exit(1);
    }

    //start of preparation
    l3pp_t l3;
    set_cpu_range(LOWER_CPU, UPPER_CPU);

    //***** lock the mapping *****
    printf("Receiver waiting for mapping...\n");
    sem_wait(sem_mapping);
    printf("Receiver mapping\n");

    prepare_receiver(&l3);

    printf("Receiver exiting  mapping...\n");
    sem_post(sem_mapping);
    //***** unlock the mapping *****

    const int numOfSlices = l3_getSlices(l3);
    uint16_t* res = (uint16_t*) calloc(MESSAGE_SIZE * numOfSlices, sizeof(uint16_t));
    for (int i = 0; i < MESSAGE_SIZE; i+= 2048/sizeof(uint16_t)) {
        res[i] = 1;
    }
    //end of preparation

    FILE *file = fopen(RECEIVER_LOG, "w"); //empty log file
    if (!file) {
        perror("Failed to open log file");
        l3_unmonitorall(l3);
        l3_release(l3);
        free(res);
        return 1;
    }
    fclose(file);

    uint16_t *tempRes = (uint16_t*) calloc(1, sizeof(uint16_t)); //size 1 is for searching for the set
    uint16_t *message = (uint16_t*) calloc(MESSAGE_SIZE, sizeof(uint16_t));

    printf("\n--------starting probe--------\n");
    uint64_t maxTime = 0;
    for ( int setNum = 0; setNum < NUM_OF_LLC_SETS; setNum++)
    {
        l3_unmonitorall(l3);
        l3_monitor(l3, setNum);
        for (int i = 0;i < MESSAGE_SIZE; i++)
        {
            uint64_t start = rdtscp64()/CLOCK_NORMALIZER;

            // sem_wait(sem_turn_receiver);
            l3_probecount(l3, tempRes);
            uint64_t end = rdtscp64()/CLOCK_NORMALIZER;
            // if (end - start > maxTime) {
            //     maxTime = end - start;
            // }
            // sem_post(sem_turn_sender);
            while (rdtscp64() < start + PROBE_CYCLES) {} //make the probe last for PROBE_CYCLES

            res[i] = tempRes[0];

        }
        // sumTime += end - start;
        // avgTime = sumTime/(setNum + 1);

        // log_time(RECEIVER_LOG, "RECEIVER MAX MESSAGE TIME ", maxTime);
        // log_time(RECEIVER_LOG, "RECEIVER MESSAGE TIME ", avgTime);
        restore_message(res, message, 1);
        if (is_restored(message, 1) == 1)
        {
            printf("SETNUM %d\n", setNum);
            print_res(res, 1);
            break;
        }
    }
    // log_time(RECEIVER_LOG, "RECEIVER MAX MESSAGE TIME ", maxTime);




    // for (int setNum = 0; setNum < NUM_OF_SETS_IN_SLICE; setNum++ )
    // {
    //     iterNum++;
    //     for (int j = 0; j < MESSAGE_SIZE; j++) {
    //         for (int slice = 0; slice < numOfSlices; slice++) {
    //             l3_unmonitorall(l3);
    //             l3_monitor(l3, SET_INDEX + slice * NUM_OF_SETS_IN_SLICE);
    //
    //             //***** wait for sender to end prime ******
    //             // printf("Receiver waiting for probe");
    //             sem_wait(sem_turn_receiver);
    //             uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
    //             l3_probecount(l3, tempRes);
    //             end = rdtscp64()/CLOCK_NORMALIZER;
    //             sem_post(sem_turn_sender);
    //             //***** signal end of probe *****
    //
    //             res[slice * MESSAGE_SIZE + j] = tempRes[0];
    //
    //         }
    //
    //         // while (rdtscp64() < start + PROBE_CYCLES) {} //make the probe last for a second
    //
    //     }
    //
    //     restore_message(res, message, numOfSlices);
    //
    //     if (is_restored(message, numOfSlices) == 1) {
    //         printf("ITERNUM %d\n", iterNum);
    //         print_res(res, numOfSlices);
    //
    //         break;
    //     }
    // }
    printf("--------probe ended--------\n\n");

    // restore_message(res, message, numOfSlices);
    stream_message_to_file(message, numOfSlices);

    l3_unmonitorall(l3);
    l3_release(l3);
    free(res);
    free(message);
    free(tempRes);

    return 0;
}