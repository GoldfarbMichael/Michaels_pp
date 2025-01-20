//
// Created by michael on 1/13/25.
//
#include <stdio.h>
#include "shared.h"



void log_time(const char *filename, const char *event, uint64_t time) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s: %lu\n", event, time);
    fclose(file);
}
