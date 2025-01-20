
#include <stdio.h>
#include <stdlib.h>

#include "prepare_receiver.h"

void prepare_receiver(l3pp_t *l3) {
    int numOfSets = 0;
    delayloop(3000000000U);
    l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
    // cache mapping for the receiver
    while (numOfSets != 12288) {
        *l3 = l3_prepare(l3i, NULL);
        numOfSets = l3_getSets(*l3);
    }

    // const int numOfSlices = l3_getSlices(*l3);
    //
    // monitor_sets(*l3, numOfSlices);
    //
    // printf("Num of sets of the RECEIVER: %d\n", numOfSets);
    // printf("Num of slices of the RECEIVER: %d\n", numOfSlices);


    free(l3i);
}


void monitor_sets(l3pp_t l3, int numOfSlices) {
    for (int slice = 0; slice < numOfSlices; slice++) {
        l3_monitor(l3, 1024*slice + SET_INDEX); //monitor all sets
        printf("monitored set: %d\n", 1024*slice + SET_INDEX);
    }
}