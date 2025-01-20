
#include <stdio.h>
#include <stdlib.h>
#include "correlated_set.h"



/*
 *returns the head of the linked list at setIndex
 *linked list of addresses
 *the linked list is a circular linked list
*/
void *getHead(void *l3, int setIndex) {
    // Validate input
    if (!l3) {
        fprintf(stderr, "Error: l3 pointer is NULL.\n");
        return NULL;
    }

    // Access `nmonitored` using its offset
    int *nmonitored_ptr = (int *)((uintptr_t)l3 + OFFSET_NMONITORED);
    int nmonitored = *nmonitored_ptr;

    // Check if setIndex is within bounds
    if (setIndex < 0 || setIndex >= nmonitored) {
        fprintf(stderr, "Error: setIndex is out of bounds. Valid range is 0 to %d.\n", nmonitored - 1);
        return NULL;
    }

    // Access `monitoredhead` using its offset
    void **monitoredhead_ptr = (void **)((uintptr_t)l3 + OFFSET_MONITOREDHEAD);

    // Return the pointer to the head of the linked list at setIndex
    return monitoredhead_ptr[0] + setIndex * sizeof(void *);

}

/*
 * Configures the linked list to be reversed
 * Assumes the linked list is circular
 */
void configure_reversed_linked_list(void **head_ptr) {
    void *head = *head_ptr;
    void *current = head;

    do {
        void *next = LNEXT(current);
        *((void **)OFFSET(next, sizeof(void *))) = current; // Set reverse link
        current = next;
    } while (current != head);
}


/*
 * Traverse the linked list 1 time forwards and 1 time backwards
 * Assumes the linked list is circular
 */
void traverse_monitored_addresses(void **head_ptr) {
    for (int i = 0; i < 10; i++) {
        if (!head_ptr) {
            printf("The linked list is empty.\n");
            return;
        }
        void *head = *head_ptr;
        void *current = head;
        do {
            current = LNEXT(current);
        } while (current && current != head); // Continue until we loop back to the head
        current = *head_ptr;
        do {
            current = LNEXT(NEXTPTR(current));
        }while (current && current != head); // Continue until we loop back to the head
    }
}



void steam_message_to_file(const uint8_t *message, FILE *file) {

    // Write bits to the file
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        fputc(message[i] ? '1' : '0', file); // Write '1' or '0' as a character
    }
    //new line for next iter
    fputc('\n', file);

    fclose(file);
}

void create_monitored_set_array(l3pp_t l3, int numOfSlices) {
    int j = 0;
    for (int slice = 0; slice < numOfSlices; slice++) {
        for (int i = 0; i < MESSAGE_SIZE; i++) {
            l3_monitor(l3, 1024 * slice + i); //monitor relevant sets at each slice
        }
    }
}


uint64_t get_time_to_traverse(void *head) {
    uint64_t start = rdtscp64();
    traverse_monitored_addresses(head);
    return rdtscp64() - start;
}


/*
 * Accesses the set in the L3 cache sets that are monitored
 * **note: there is an assumption that the MESSAGE_SIZE = nMonitored
 *         done NUM_OF_MESSAGE_SENDS times
 *         every round happens after a slot time (of cycles)
 */
void correlated_prime(void* head, const uint8_t bit, int slot, uint64_t traverseTime) {
    if (bit == 1) {
        traverse_monitored_addresses(head);
    }
    else {
        slotwait(traverseTime); //wait for the time to traverse if there is no need to traverse
    }
}

void create_message(uint8_t *message) {
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        if (i%2 == 0)
            message[i] = 1;
        else
            message[i] = 0;
    }
}

void prepare_sender(l3pp_t* l3, uint8_t* message) {
    int numOfSets = 0;
    l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
    // cache mapping for the receiver

    while (numOfSets != 12288) {
        *l3 = l3_prepare(l3i, NULL);
        numOfSets = l3_getSets(*l3);
    }

    create_message(message);
    printf("Finished mapping and Message has been created\n");
    l3_unmonitorall(*l3);
    l3_monitor(*l3, SET_INDEX);


    configure_reversed_linked_list(getHead(*l3, 0));
    printf("Reversed Linked list has been configured\n");

    //stream the message into a txt file
    FILE *file = fopen("messageSent.txt", "w");
    if (!file) {
        perror("Failed to open output file");
        return;
    }

    // Write bits to the file
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        fputc(message[i] ? '1' : '0', file); // Write '1' or '0' as a character
    }

    fclose(file);
    free(l3i);
}

