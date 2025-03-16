#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mastik/l3.h>
#include <mastik/impl.h>
#include "correlated_set.h"
#include <sched.h>
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LOWER_CPU 4
#define UPPER_CPU 4
#define CLOCK_NORMALIZER 1
#define SENDER_LOG "../../cmake-build-debug/PrimeProbe/sender_log.log"
#define PRIME_CYCLES (2600000000/10000)   // should be a second

#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"


void openLink(char* url) {
    char command[2048];
    snprintf(command, sizeof(command), "taskset -c 0 firefox %s &", url);
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

void log_time(const char *filename, const char *event, uint64_t time) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s %lu\n", event, time);
    fclose(file);
}



int main(int argc, char *argv[]) {
    sem_t *sem_mapping = sem_open(SEM_MAPPING, O_RDWR);
    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_SENDER, O_RDWR);
    if (sem_turn_sender == SEM_FAILED || sem_turn_receiver == SEM_FAILED || sem_mapping == SEM_FAILED) {
        perror("sem_open failed");
        exit(1);
    }

    //start of sender preparation
    l3pp_t l3;
    uint8_t *message = calloc(MESSAGE_SIZE, sizeof(uint8_t));

    set_cpu_range(LOWER_CPU, UPPER_CPU);

    FILE *file = fopen(SENDER_LOG, "w"); //empty log file
    if (!file) {
        perror("Failed to open log file");
        return 1;
    }
    fclose(file);

    //***** lock the mapping *****
    printf("sender waiting for mapping...\n");
    sem_wait(sem_mapping);
    printf("sender mapping\n");

    prepare_sender(&l3, message);
    void* monitoredHead = getHead(l3, 0);

    // monitor_all_sets(&l3); // ****************** Necessary for priming all sets ************
    l3_unmonitorall(l3);
    l3_monitor(l3, SET_INDEX);
    printf("sender exiting mapping...\n");

    // uint64_t traverseTime = get_time_to_traverse(monitoredHead);
    sem_post(sem_mapping);
    //***** unlock the mapping *****


    printf("----------------started priming----------------\n");

    for (int setNum = 0; setNum < NUM_OF_LLC_SETS; setNum++) //iterates only on SET_INDEX but does it NUM_OF_LLC_SETS times
    {
        for (int i = 0; i < MESSAGE_SIZE; i++)
        {
            sem_wait(sem_turn_sender);
            prime_all_sets(&l3, message[i]);
            sem_post(sem_turn_receiver);
        }
    }



    // for (int setNum = 0; setNum < NUM_OF_SETS_IN_SLICE; setNum++ )
    // {
    //
    //     for (int i =0; i < MESSAGE_SIZE; i++) {
    //         for (int round = 0; round < l3_getSlices(l3); round++) {
    //
    //             //***** wait for receiver to end probe ******
    //             sem_wait(sem_turn_sender);
    //             uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
    //
    //             prime_all_sets(&l3, message[i]);
    //
    //             uint64_t end = rdtscp64()/CLOCK_NORMALIZER;
    //             sem_post(sem_turn_receiver);
    //             //***** signal end of prime *****
    //
    //         }
    //     }
    // }
    printf("---------------- priming ended----------------\n");

    free(message);
    l3_release(l3);
    return EXIT_SUCCESS;
}