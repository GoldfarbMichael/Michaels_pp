
#ifndef CORRELATED_SET_H
#define CORRELATED_SET_H


#include <mastik/l3.h>
#include <mastik/impl.h>
#include <mastik/util.h>


#define MESSAGE_SIZE 10
#define SET_INDEX 6
#define OFFSET_MONITOREDHEAD 0
#define OFFSET_NMONITORED 8
#define NUM_OF_MESSAGE_SENDS 1000000

void *getHead(void *l3, int setIndex);
void traverse_monitored_addresses(void **head_ptr);
void correlated_prime(void* head, uint8_t bit, int slot, uint64_t traverseTime);
void create_message(uint8_t *message);
void prepare_sender(l3pp_t* l3, uint8_t* message);
uint64_t get_time_to_traverse(void *head);


#endif //CORRELATED_SET_H
