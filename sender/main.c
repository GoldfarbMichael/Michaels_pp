#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mastik/l3.h>
#include <mastik/impl.h>
#include "correlated_set.h"
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#define LOWER_CPU 4
#define UPPER_CPU 4
#define TSC_FREQ 3600000000ULL
#define TSC_OFFSET 100000000ULL        // start 100M cycles after epoch (≈33ms)
#define CLOCK_NORMALIZER 36000
#define SENDER_LOG "../../cmake-build-debug/PrimeProbe/sender_log.csv"
#define PRIME_CYCLES (TSC_FREQ/CLOCK_NORMALIZER)   // should be a second

#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Missing start_time argument\n");
        return 1;
    }
    uint64_t startPP = strtoull(argv[1], NULL, 10);
    sem_t *sem_mapping = sem_open(SEM_MAPPING, O_RDWR);
    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_SENDER, O_RDWR);


    if (sem_turn_sender == SEM_FAILED || sem_turn_receiver == SEM_FAILED || sem_mapping == SEM_FAILED) {
        perror("sem_open failed");
        exit(1);
    }

    //start of sender preparation
    l3pp_t l3;
    uint16_t *message = calloc(MESSAGE_SIZE, sizeof(uint16_t));

    set_cpu_range(LOWER_CPU, UPPER_CPU);
    printf("SENDER RUNS ON CORE NUM: ");
    print_core();


    //***** lock the mapping *****
    printf("sender waiting for mapping...\n");
    sem_wait(sem_mapping);
    printf("sender mapping\n");

    prepare_sender(&l3, message);

    l3_unmonitorall(l3);
    l3_monitor(l3, SET_INDEX);
    printf("sender exiting mapping...\n");
    sem_post(sem_mapping);
    //***** unlock the mapping *****

    printf("----------------started priming----------------\n");
    printf("TIME AT SENDER OUTSIDE+++++ %lu\n", rdtscp64());

        for ( int setNum = 0; setNum < NUM_OF_LLC_SETS; setNum++) //iterates only on SET_INDEX but does it NUM_OF_LLC_SETS times
        {
            for (int i = 0; i < MESSAGE_SIZE; i++)
            {
                // sem_wait(sem_turn_sender);
                while (rdtscp64() < startPP) {asm volatile("pause");}
                prime_monitored_sets(&l3, message[i]);
                startPP += 70000;
                // sem_post(sem_turn_receiver);
            }

        }
    printf("---------------- priming ended----------------\n");
    free(message);
    l3_release(l3);
    return EXIT_SUCCESS;
}