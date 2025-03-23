//
// Created by michael on 12/25/24.
//

#ifndef PREPARE_RECEIVER_H
#define PREPARE_RECEIVER_H

#include <mastik/l3.h>
#include <mastik/impl.h>
#include <mastik/util.h>

#define SET_INDEX 6
#define MESSAGE_SIZE 1000
#define NUM_OF_LLC_SETS 16384
#define NUM_OF_SETS_IN_SLICE 2048

void prepare_receiver(l3pp_t *l3);

void monitor_sets(l3pp_t l3, int numOfSlices); // Monitors correlated group of sets in every slice
#endif //PREPARE_RECEIVER_H
