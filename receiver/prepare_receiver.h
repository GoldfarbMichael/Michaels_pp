//
// Created by michael on 12/25/24.
//

#ifndef PREPARE_RECEIVER_H
#define PREPARE_RECEIVER_H

#include <mastik/l3.h>
#include <mastik/impl.h>
#include <mastik/util.h>

#define SET_INDEX 6
#define MESSAGE_SIZE 512
// #define MESSAGE_STR "The quick brown fox jumps over 13 lazy dogs! Stealthy bits whisper secrets through silent L3 shadows. Keep it hidden please bla."
#define MESSAGE_STR "The quick brown fox jumps over 13 lazy dogs!Stealthy bits secret"
#define NUM_OF_LLC_SETS 16384
#define NUM_OF_SETS_IN_SLICE 2048

void prepare_receiver(l3pp_t *l3);

void monitor_sets(l3pp_t l3, int numOfSlices); // Monitors correlated group of sets in every slice
#endif //PREPARE_RECEIVER_H
