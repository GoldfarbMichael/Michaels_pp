#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "prepare_receiver.h"


void stream_message_to_file(const uint16_t *message, const int numOfSlices) {
    //check for nulls
    if (!message) {
        fprintf(stderr, "Error: NULL pointer passed to steam_message_to_file.\n");
        return;
    }

    FILE *file = fopen("recoveredMessage.txt", "w");
    if (!file) {
        perror("Failed to open output file");
        return;
    }
    // Write bits to the file
    int index = 0;
    for (int sliceNum = 0; sliceNum < numOfSlices; sliceNum++) {
        fprintf(file, "Slice number: %d\n", sliceNum);
        for (int i = 0; i < MESSAGE_SIZE; i++) {
            fprintf(file, "%d", (int16_t)(message[index])); // Write the number in the cell
            index++;

        }
        fprintf(file,"\n"); // New line for the next slice
    }
    fclose(file);
}




void print_res(const uint16_t *res, const int numOfSlices) {
    // Write the repetition number to the file
    int index = 0;
    for (int sliceNum = 0; sliceNum < numOfSlices; sliceNum++) {
        printf("Slice number: %d\n ", sliceNum);
        for (int i = 0; i < MESSAGE_SIZE; i++) {
            printf("%4d ", (int16_t)(res[index])); // Write the number in the cell
            index++;

        }
        printf("\n"); // New line for the next slice
    }
}

