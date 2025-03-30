#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>    // For O_CREAT
#include <mastik/low.h>

#include "shared.h"

#define SENDER_LOG "../../cmake-build-debug/PrimeProbe/sender_log.log"
#define RECEIVER_LOG "../../cmake-build-debug/PrimeProbe/receiver_log.log"
#define MESSAGE_SIZE 20
#define LOWER_CPU 7
#define UPPER_CPU 7
#define SEM_TURN_SENDER "/sem_turn_sender"
#define SEM_TURN_RECEIVER "/sem_turn_receiver"
#define SEM_MAPPING "/sem_mapping"
#define START_DELAY (5*3600000000ULL)
typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t took; // Optional, can remain 0 if not present
} LogData;


int parse_log_file(const char *file_path, LogData log_data[MESSAGE_SIZE]) {
    FILE *log_file = fopen(file_path, "r");
    if (!log_file) {
        perror("Failed to open log file");
        return -1;
    }

    char line[256];
    int round = 0;

    while (fgets(line, sizeof(line), log_file)) {
        if (round >= MESSAGE_SIZE) {
            fprintf(stderr, "Warning: More rounds in the file than expected (%d)\n", MESSAGE_SIZE);
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


void initialize_semaphore(sem_t **sem, const char *name, int initial_value) {
    sem_unlink(name);  // Ensure it's removed before re-creating it
    *sem = sem_open(name, O_CREAT | O_RDWR, 0666, initial_value);
    if (*sem == SEM_FAILED) {
        perror("sem_open failed");
        exit(1);
    }
}

int main() {
    pid_t receiver_pid, sender_pid;

    set_cpu_range(LOWER_CPU, UPPER_CPU);

    char* senderPath = "../../cmake-build-debug/sender/sender";
    char* receiverPath = "../../cmake-build-debug/receiver/receiver";


    sem_t *sem_turn_receiver, *sem_turn_sender, *sem_mapping, *sem_nextSet;

    // Initialize named semaphores
    initialize_semaphore(&sem_mapping, SEM_MAPPING, 1);
    initialize_semaphore(&sem_turn_sender, SEM_TURN_SENDER, 1);
    initialize_semaphore(&sem_turn_receiver, SEM_TURN_RECEIVER, 0); // Receiver must wait
    uint64_t startPP = rdtscp64() + START_DELAY;
    char startPPstr[32];
    sprintf(startPPstr, "%lu", startPP);

    // Fork the second child process to execute ./sender
    sender_pid = fork();
    if (sender_pid < 0) {
        perror("Failed to fork sender process");
        exit(EXIT_FAILURE);
    }

    if (sender_pid == 0) {
        // In the child process for sender
        printf("Starting sender process...\n\n");
        execl(senderPath, "./sender", startPPstr, NULL);
        // If execl returns, there was an error
        perror("Failed to execute ./sender");
        exit(EXIT_FAILURE);
    }




    // Fork the first child process to execute ./receiver
    receiver_pid = fork();
    if (receiver_pid < 0) {
        perror("Failed to fork receiver process");
        exit(EXIT_FAILURE);
    }


    if (receiver_pid == 0) {
        // In the child process for receiver
        printf("Starting receiver process...\n");
        execl(receiverPath, "./receiver",startPPstr , NULL);
        // If execl returns, there was an error
        perror("Failed to execute ./receiver");
        exit(EXIT_FAILURE);
    }




    // In the parent process: wait for both child processes to finish
    int receiver_status, sender_status;


    waitpid(receiver_pid, &receiver_status, 0); // Wait for receiver
    printf("Receiver process finished with status %d\n", WEXITSTATUS(receiver_status));


    // Terminate the sender process
    if (kill(sender_pid, SIGTERM) != 0) {
        perror("Failed to terminate sender process");
    }

    waitpid(sender_pid, &sender_status, 0); // Wait for sender
    printf("Sender process finished with status %d\n", WEXITSTATUS(sender_status));


    // Cleanup semaphores
    sem_unlink(SEM_TURN_RECEIVER);
    sem_unlink(SEM_TURN_SENDER);
    sem_unlink(SEM_MAPPING);


    return 0;
}
