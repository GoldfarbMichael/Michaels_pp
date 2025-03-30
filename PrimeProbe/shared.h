#ifndef SHARED_H
#define SHARED_H
#include <stdint.h>

void set_cpu_range(int start_cpu, int end_cpu);
void log_time(const char *filename, const char *event, uint64_t time);
void log_start_time(const char *filename, uint64_t time);
void print_string_from_bits(const uint16_t *bit_array);
void print_core();



#endif //SHARED_H
