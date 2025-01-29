
#include <stdlib.h>
#include "training.h"
#include "prepare_receiver.h"
#define NUM_OF_TESTS 20
#define SLOT 1

uint16_t calculate_avg_misses(const uint16_t* res, int resSize) {
    int sum = 0;
    for (int i = 0; i < resSize; i++) {
        sum += res[i];
    }
    return sum/resSize;
}

int find_min_index(const uint16_t* res, int resSize) {
    int min = res[0];
    int minIndex = 0;
    for (int i = 1; i < resSize; i++) {
        if (res[i] < min) {
            min = res[i];
            minIndex = i;
        }
    }
    return minIndex;
}

void find_most_silent_set(l3pp_t* l3) {

    int numOfSets = l3_getSets(*l3);
    const int numOfSlices = l3_getSlices(*l3);
    const int setsInSlice = numOfSets/numOfSlices;
    // printf("Num of sets in a slice:%d \n", setsInSlice);

    uint16_t* avgRes = (uint16_t*) calloc(setsInSlice, sizeof(uint16_t));
    uint16_t* res = (uint16_t*) calloc(NUM_OF_TESTS, sizeof(uint16_t));

    for (int i = 0; i < NUM_OF_TESTS; i+= NUM_OF_SETS_IN_SLICE/sizeof(uint16_t)) {
        res[i] = 1;
    }

    for (int i = 0; i < setsInSlice; i++) {
        l3_unmonitorall(*l3);
        l3_monitor(*l3, i);
        l3_repeatedprobecount(*l3, NUM_OF_TESTS, res, SLOT);
        // printf("Set Num: %d\n", i);
        // for (int j = 0; j < NUM_OF_TESTS; j++) {
        //     printf("%4d ", (int16_t)res[j]);
        // }
        // putchar('\n');
        // avgRes[i] = calculate_avg_misses(res, NUM_OF_TESTS);
    }
    // int silentIndex = find_min_index(avgRes, setsInSlice);
    // printf("The most silent set is: %d\n", silentIndex);
    // printf("The AVG is: %d\n", avgRes[silentIndex]);
    // for (int i = 0; i < setsInSlice; i++) {
    //     printf("%4d ", (int16_t)avgRes[i]);
    // }


    free(avgRes);
    free(res);
}


