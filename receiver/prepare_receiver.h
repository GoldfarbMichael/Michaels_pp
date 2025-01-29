//
// Created by michael on 12/25/24.
//

#ifndef PREPARE_RECEIVER_H
#define PREPARE_RECEIVER_H

#include <mastik/l3.h>
#include <mastik/impl.h>
#include <mastik/util.h>

#define SET_INDEX 10
#define REPS 10
#define MESSAGE_SIZE 50

void prepare_receiver(l3pp_t *l3);

void monitor_sets(l3pp_t l3, int numOfSlices); // Monitors correlated group of sets in every slice
#endif //PREPARE_RECEIVER_H
