#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <mastik/l3.h>
#include <mastik/impl.h>
#include "correlated_set.h"
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LOWER_CPU 4
#define UPPER_CPU 4
#define CLOCK_NORMALIZER (1)
#define SENDER_LOG "../../cmake-build-debug/PrimeProbe/sender_log.log"
#define PRIME_CYCLES (3600000000/CLOCK_NORMALIZER)   // should be a second

#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"
#define SEM_NEXT_SET "/sem_nextSet"


int main(int argc, char *argv[]) {
    sem_t *sem_mapping = sem_open(SEM_MAPPING, O_RDWR);
    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_SENDER, O_RDWR);
    sem_t *sem_nextSet = sem_open(SEM_NEXT_SET, O_RDWR);


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
    uint64_t sumTime = 0;
    uint64_t avgTime = 0;
    uint64_t maxTime = 0;
    int setIndex = 0;
        for ( int setNum = 0; setNum < NUM_OF_LLC_SETS; setNum++) //iterates only on SET_INDEX but does it NUM_OF_LLC_SETS times
        {
            // l3_unmonitorall(l3);
            // l3_monitor(l3, setIndex);
            // printf("setIndex: %d\n", setIndex);
            for (int i = 0; i < MESSAGE_SIZE; i++)
            {
                // uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
                sem_wait(sem_turn_sender);
                prime_monitored_sets(&l3, message[i]);
                sem_post(sem_turn_receiver);
                // uint64_t end = rdtscp64()/CLOCK_NORMALIZER;

                // while (rdtscp64() < start + PRIME_CYCLES) {} //make the probe last for PRIME_CYCLES

            }
            // sumTime += end - start;
            // avgTime = sumTime/(setNum + 1);

            // log_time(SENDER_LOG, "SENDER MESSAGE TIME ", avgTime);
            // log_time(SENDER_LOG, "SENDER MAX MESSAGE TIME ", maxTime);


            // if (sem_trywait(sem_nextSet) == 0) {
            //     // Critical section
            //     setIndex++;
            //     sem_post(sem_nextSet);
            //     break;
            //
            // }
        }
        // printf("waiting in the sender...\n");
        // sem_wait(sem_nextSet);
        // // Critical section
        // setIndex++;
        // sem_post(sem_nextSet);
        // printf("relesed in the sender...\n");

    printf("---------------- priming ended----------------\n");

    free(message);
    l3_release(l3);
    return EXIT_SUCCESS;
}