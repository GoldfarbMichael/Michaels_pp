#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#define SYNC_FILE_R "/tmp/rceiver_prepared"
#define SYNC_FILE_S "/tmp/sender_prepared"

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
    // sleep(5U);

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
        printf("Starting sender process...\n");
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

    // Cleanup the sync file
    unlink(SYNC_FILE_S);
    unlink(SYNC_FILE_R);

    printf("Sync file cleaned up.\n");
    return 0;
}
