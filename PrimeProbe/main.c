#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdint.h>
#include <string.h>
#define SYNC_FILE_R "/tmp/rceiver_prepared"
#define SYNC_FILE_S "/tmp/sender_prepared"
#define SENDER_LOG "/home/michael/CLionProjects/Michaels_PP/cmake-build-debug/PrimeProbe/sender_log.log"
#define RECEIVER_LOG "/home/michael/CLionProjects/Michaels_PP/cmake-build-debug/PrimeProbe/receiver_log.log"
#define NUM_ROUNDS 12
#define LOWER_CPU 10
#define UPPER_CPU 11

void set_cpu_range(int start_cpu, int end_cpu) {
    cpu_set_t set;
    CPU_ZERO(&set); // Clear the CPU mask

    // Add CPUs in the specified range to the mask
    for (int i = start_cpu; i <= end_cpu; i++) {
        CPU_SET(i, &set);
    }

    // Apply the CPU affinity to the current process
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) != 0) {
        perror("sched_setaffinity");
    }
}

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t took; // Optional, can remain 0 if not present
} LogData;


int parse_log_file(const char *file_path, LogData log_data[NUM_ROUNDS]) {
    FILE *log_file = fopen(file_path, "r");
    if (!log_file) {
        perror("Failed to open log file");
        return -1;
    }

    char line[256];
    int round = 0;

    while (fgets(line, sizeof(line), log_file)) {
        if (round >= NUM_ROUNDS) {
            fprintf(stderr, "Warning: More rounds in the file than expected (%d)\n", NUM_ROUNDS);
            break;
        }

        if (strstr(line, "start")) {
            if (sscanf(line, "%*s %*s %lu", &log_data[round].start) != 1) {
                fprintf(stderr, "Error: Failed to parse 'start' in round %d\n", round);
            }
        } else if (strstr(line, "end")) {
            if (sscanf(line, "%*s %*s %lu", &log_data[round].end) != 1) {
                fprintf(stderr, "Error: Failed to parse 'end' in round %d\n", round);
            }
        } else if (strstr(line, "took")) {
            if (sscanf(line, "%*s %*s %lu", &log_data[round].took) != 1) {
                fprintf(stderr, "Error: Failed to parse 'took' in round %d\n", round);
            }
        } else if (strstr(line, "------------------------------------ 0")) {
            round++; // Move to the next round
        }
    }

    fclose(log_file);
    return 0; // Success
}


// Function to read times from a log file
void read_times(const char *filename, uint64_t *start, uint64_t *end) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open log file");
        return;
    }
    char event[20];
    while (fscanf(file, "%s: %ld\n", event, start) != EOF) {
        if (strcmp(event, "start") == 0) {
            *start = *start;
        } else if (strcmp(event, "end") == 0) {
            *end = *end;
        }
    }
    fclose(file);
}


int main() {
    pid_t receiver_pid, sender_pid;
    set_cpu_range(LOWER_CPU, UPPER_CPU);
    char* senderPath = "/home/michael/CLionProjects/Michaels_PP/cmake-build-debug/sender/sender";
    char* receiverPath = "/home/michael/CLionProjects/Michaels_PP/cmake-build-debug/receiver/receiver";



    // Fork the first child process to execute ./receiver
    receiver_pid = fork();
    if (receiver_pid < 0) {
        perror("Failed to fork receiver process");
        exit(EXIT_FAILURE);
    }


    if (receiver_pid == 0) {
        // In the child process for receiver
        printf("Starting receiver process...\n");
        execl(receiverPath, "./receiver", NULL);
        // If execl returns, there was an error
        perror("Failed to execute ./receiver");
        exit(EXIT_FAILURE);
    }

    // Fork the second child process to execute ./sender
    sender_pid = fork();
    if (sender_pid < 0) {
        perror("Failed to fork sender process");
        exit(EXIT_FAILURE);
    }

    if (sender_pid == 0) {
        // In the child process for sender
        printf("Starting sender process...\n\n");
        execl(senderPath, "./sender", NULL);
        // If execl returns, there was an error
        perror("Failed to execute ./sender");
        exit(EXIT_FAILURE);
    }



    // In the parent process: wait for both child processes to finish
    int receiver_status, sender_status;


    waitpid(receiver_pid, &receiver_status, 0); // Wait for receiver
    printf("Receiver process finished with status %d\n", WEXITSTATUS(receiver_status));

    waitpid(sender_pid, &sender_status, 0); // Wait for sender
    printf("Sender process finished with status %d\n", WEXITSTATUS(sender_status));

    LogData sender_data[NUM_ROUNDS];
    LogData receiver_data[NUM_ROUNDS];

    parse_log_file(SENDER_LOG, sender_data);
    parse_log_file(RECEIVER_LOG, receiver_data);

    // Print the times and the distance between the starts
    for (int i = 0; i < NUM_ROUNDS; i++) {
        printf("----------------ROUND %d----------------\n", i+1);
        printf("Sender start: %lu\nSender end: %lu\nSender took: %lu\n\n", sender_data[i].start, sender_data[i].end, sender_data[i].took);
        printf("Receiver start: %lu\nReceiver end: %lu\nReceiver took: %lu\n\n", receiver_data[i].start, receiver_data[i].end, receiver_data[i].took);
        printf("----Distances (sender - receiver)----\nDistance between starts: %ld \n", sender_data[i].start - receiver_data[i].start);
        printf("Distance between ends: %ld \n", sender_data[i].end- receiver_data[i].end);
        printf("Took distance: %ld \n", sender_data[i].took- receiver_data[i].took);
        printf("(SenderEnd - ReceiverStart): %ld \n", sender_data[i].end - receiver_data[i].start);
        if (i > 0) {
            printf("(ReceiverEndPrev - SenderStartCurr): %ld \n", receiver_data[i-1].end - sender_data[i].start);
        }


    }


    // Cleanup the sync file
    unlink(SYNC_FILE_S);
    unlink(SYNC_FILE_R);

    printf("Sync file cleaned up.\n");
    return 0;
}
