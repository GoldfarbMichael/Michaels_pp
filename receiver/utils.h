#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>


void stream_message_to_file(const uint16_t *message, const int numOfSlices);
void print_res(const uint16_t *res, const int numOfSlices);

#endif //UTILS_H
