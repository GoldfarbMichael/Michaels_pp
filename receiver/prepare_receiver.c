
#include <stdio.h>
#include <stdlib.h>

#include "prepare_receiver.h"

void prepare_receiver(l3pp_t *l3) {
    int numOfSets = 0;
    delayloop(3000000000U);
    l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
    // cache mapping for the receiver
    while (numOfSets != NUM_OF_LLC_SETS) {
        *l3 = l3_prepare(l3i, NULL);
        numOfSets = l3_getSets(*l3);
    }



    free(l3i);
}


void monitor_sets(l3pp_t l3, int numOfSlices) {
    for (int slice = 0; slice < numOfSlices; slice++) {
        l3_monitor(l3, NUM_OF_SETS_IN_SLICE*slice + SET_INDEX); //monitor all sets
        printf("monitored set: %d\n", NUM_OF_SETS_IN_SLICE*slice + SET_INDEX);
    }
}