
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <mastik/util.h>
#include <mastik/low.h>
#include <mastik/l3.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "prepare_receiver.h"
#include "training.h"
#include "utils.h"
#include "../PrimeProbe/shared.h"
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

#define MISS_THRESHOLD 5
#define LOWER_CPU 0
#define UPPER_CPU 0
#define CLOCK_NORMALIZER (1)
#define RECEIVER_LOG "../../cmake-build-debug/PrimeProbe/receiver_log.log"
#define PROBE_CYCLES (3600000000/CLOCK_NORMALIZER)  // should be a second

#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"
#define SEM_NEXT_SET "/sem_nextSet"


uint64_t get_probe_time(l3pp_t l3, uint16_t *res) {
    uint64_t start = rdtscp64();
    l3_probe(l3, res);
    return rdtscp64() - start;
}


void evict_monitor_and_evict_sets(l3pp_t l3, int numOfSlices) {
    uint16_t* res = (uint16_t*) calloc(numOfSlices * MESSAGE_SIZE, sizeof(uint16_t));
    for (int i = 0; i < numOfSlices; i++) {
        l3_monitor(l3, SET_INDEX + i * NUM_OF_SETS_IN_SLICE);
    }
    l3_repeatedprobe(l3, 20, res, 1);
    free(res);
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

void find_expected_message(uint8_t *searched_seq)
{
    for (size_t i = 0; i < MESSAGE_SIZE; i++) {
        char c = MESSAGE_STR[i];
        for (int b = 7; b >= 0; b--) {
            searched_seq[i * 8 + (7 - b)] = (c >> b) & 1;
        }
    }
}


/**
 *
 * @param restoredMessage the restored message (size of the message is MESSAGE_SIZE * NumOfSlices)
 * @param expectedMessage the expected binary message
 * @param NumOfSlices number of slices
 * @param accuracy the desired accuracy in order to declare that the message restored correctly
 * @return returns true if the message is restored at with at least "accuracy" accuracy
 **/
int is_restored(const uint16_t *restoredMessage, const uint8_t *expectedMessage, int NumOfSlices, const float accuracy)
{
    for (int sliceNum = 0; sliceNum < NumOfSlices; sliceNum++)
    {
        float sum = 0;
        for (int i = 0; i < MESSAGE_SIZE; i++)
        {
            if (restoredMessage[sliceNum*NUM_OF_SETS_IN_SLICE + i] == (uint16_t)expectedMessage[i])
                sum++;
            else
                sum --;
        }
        if (sum / (float)MESSAGE_SIZE >= accuracy)
        {
            printf("THE SUM IS %f\n", sum);
            //print the restored message
            for (int i = 0; i < MESSAGE_SIZE; i++)
            {
                printf("%d ", restoredMessage[sliceNum*NUM_OF_SETS_IN_SLICE + i]);
                if ((i + 1) % 8 == 0) printf(" ");
                if ((i + 1) % 64 == 0) printf("\n");
            }
            print_string_from_bits(restoredMessage);
            return 1;
        }
    }
    return 0;
}


int main(int ac, char **av) {

    sem_t *sem_mapping = sem_open(SEM_MAPPING, O_RDWR);
    sem_t *sem_turn_receiver = sem_open(SEM_TURN_RECEIVER, O_RDWR);
    sem_t *sem_turn_sender = sem_open(SEM_TURN_SENDER, O_RDWR);
    sem_t *sem_nextSet = sem_open(SEM_NEXT_SET, O_RDWR);

    uint8_t expectedMessage[MESSAGE_SIZE];
    find_expected_message(expectedMessage);

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

    // for (int round = 0; round < 5; round++)
    // {
    //     // sem_wait(sem_nextSet);
    //     printf("AQUIRED AT RECIEVER %d...\n", round);
    //     for ( int setNum = 0; setNum < NUM_OF_LLC_SETS; setNum++)
    //     {
    //         l3_unmonitorall(l3);
    //         l3_monitor(l3, setNum);
    //         for (int i = 0;i < MESSAGE_SIZE; i++)
    //         {
    //             // uint64_t start = rdtscp64()/CLOCK_NORMALIZER;
    //
    //             sem_wait(sem_turn_receiver);
    //             l3_probecount(l3, tempRes);
    //             // uint64_t end = rdtscp64()/CLOCK_NORMALIZER;
    //             // if (end - start > maxTime) {
    //             //     maxTime = end - start;
    //             // }
    //             sem_post(sem_turn_sender);
    //             // while (rdtscp64() < start + PROBE_CYCLES) {} //make the probe last for PROBE_CYCLES
    //             res[i] = tempRes[0];
    //
    //         }
    //         // sumTime += end - start;
    //         // avgTime = sumTime/(setNum + 1);
    //
    //         // log_time(RECEIVER_LOG, "RECEIVER MAX MESSAGE TIME ", maxTime);
    //         // log_time(RECEIVER_LOG, "RECEIVER MESSAGE TIME ", avgTime);
    //         restore_message(res, message, 1);
    //         if (is_restored(message,expectedMessage , 1, 0.98) == 1)
    //         {
    //             printf("SETNUM %d\n", setNum);
    //             print_res(res, 1);
    //             // sem_post(sem_nextSet);
    //             // sleep(1);
    //             printf("RELEASED AT RECIEVER %d...\n", round);
    //             break;
    //         }
    //     }
    // }
    // log_time(RECEIVER_LOG, "RECEIVER MAX MESSAGE TIME ", maxTime);

    for (int round = 0; round < 5; round++)
    {
        for ( int setNum = SET_INDEX; setNum < NUM_OF_LLC_SETS; setNum+=1024)
        {
            l3_unmonitorall(l3);
            l3_monitor(l3, setNum);
            for (int i = 0;i < MESSAGE_SIZE; i++)
            {
                sem_wait(sem_turn_receiver);
                l3_probecount(l3, tempRes);
                sem_post(sem_turn_sender);
                res[i] = tempRes[0];
            }
            restore_message(res, message, 1);
            if (is_restored(message,expectedMessage , 1, 0.98) == 1)
            {
                printf("SETNUM %d -- ROUND NUM %d\n", setNum, round);
                // print_res(res, 1);
                break;
            }
        }
    }
    printf("--------probe ended--------\n\n");

    // restore_message(res, message, numOfSlices);
    // stream_message_to_file(message, numOfSlices);

    l3_unmonitorall(l3);
    l3_release(l3);
    free(res);
    free(message);
    free(tempRes);

    return 0;
}
// 5056 5346 5717 5766