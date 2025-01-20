/*
 * Copyright 2016 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <mastik/util.h>
#include <mastik/low.h>
#include <mastik/l3.h>
#define SAMPLES 200
#define SET_INDEX 0



void sender_function(l3pp_t l3, int set_index, int bit) {
    if (bit == 1) {
        // Access all cache lines in the specified set to create contention
        l3_monitor(l3, set_index);
    } else {
        // Do nothing, leaving the set unaccessed
        l3_unmonitor(l3, set_index);
    }
}

// Function to mimic receiver activity
int receiver_function(l3pp_t l3, int set_index, uint16_t *res) {
	
	//l3_unmonitorall(l3);
    l3_monitor(l3, set_index);
	sender_function(l3, SET_INDEX, 1);  // Send bit 1
	int count = l3_repeatedprobecount(l3, SAMPLES, res, 5000);
	return count;
}

int main(int ac, char **av) {
	
	delayloop(3000000000U);
    l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
    l3pp_t l3 = l3_prepare(l3i, NULL);
    uint16_t *res = calloc(SAMPLES, sizeof(uint16_t));


   int nsets = l3_getSets(l3);
	int nslices = l3_getSlices(l3);
	printf("Number of sets: %d\n", nsets);
	printf("Number of slices: %d\n", nslices);
    //initialize res array sparsly
    for (int i = 0; i < SAMPLES; i+= 2048/sizeof(uint16_t))
        res[i] = 1;


 //   sender_function(l3, SET_INDEX, 1);  // Send bit 1

    // Receive bit for all the possible slices
	for(int i = SET_INDEX ; i < nsets; i+= 1024) { //slice size times number of slices
	l3_unmonitorall(l3);
    int received_bit = receiver_function(l3, i, res);
		printf("Set index: %d --> slice number: %d \n", i, i/1024);
    	for (int j = 0; j < SAMPLES; j++) {
			printf("%4d ", (int16_t)res[j]);
		}
    	printf("\n\n");

    }
    // Cleanup
	l3_unmonitorall(l3);
	l3_release(l3);
	free(res);
	free(l3i);
    return 0;
	
	
}
	

	
	//------------------------------------------------------------------------------------------------------------------------------------------------
	
/*


typedef struct {
    l3pp_t l3;
    int set_index;
    int bit;
} sender_args_t;

typedef struct {
    l3pp_t l3;
    int set_index;
    uint16_t *res;
} receiver_args_t;

// Sender function
void* sender_function(void* args) {
    sender_args_t* sender_args = (sender_args_t*)args;
    l3pp_t l3 = sender_args->l3;
    int set_index = sender_args->set_index;
    int bit = sender_args->bit;

    if (bit == 1) {
        l3_monitor(l3, set_index); // Access to create contention
    } else {
        l3_unmonitor(l3, set_index); // Leave set unaccessed
    }

    return NULL;
}

// Receiver function
void* receiver_function(void* args) {
    receiver_args_t* receiver_args = (receiver_args_t*)args;
    l3pp_t l3 = receiver_args->l3;
    int set_index = receiver_args->set_index;
    uint16_t *res = receiver_args->res;

    l3_monitor(l3, set_index);
    int count = l3_repeatedprobecount(l3, SAMPLES, res, 5000);

    printf("Set index: %d\n", set_index);
    for (int j = 0; j < SAMPLES; j++) {
        printf("%4d ", (int16_t)res[j]);
    }
    printf("\n\n");

    return NULL;
}

int main(int ac, char **av) {
    delayloop(3000000000U);

    l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
    l3pp_t l3 = l3_prepare(l3i, NULL);
    uint16_t *res = calloc(SAMPLES, sizeof(uint16_t));

    // Initialize res array sparsely
    for (int i = 0; i < SAMPLES; i += 4096 / sizeof(uint16_t))
        res[i] = 1;

   
    pthread_t sender_thread;
	pthread_t receiver_thread;

    // sender arguments
    sender_args_t sender_args;
    sender_args.l3 = l3;
    sender_args.set_index = SET_INDEX;
    sender_args.bit = 1;

    // receiver arguments
    receiver_args_t receiver_args;
    receiver_args.l3 = l3;
    receiver_args.set_index = SET_INDEX; 
    receiver_args.res = res;

    // sender run()
    pthread_create(&sender_thread, NULL, sender_function, (void*)&sender_args);

   
    usleep(100); // delay to allow sender activity

    // receiver run()
    pthread_create(&receiver_thread, NULL, receiver_function, (void*)&receiver_args);

    // Wait for threads to finish
    pthread_join(sender_thread, NULL);
    pthread_join(receiver_thread, NULL);

    // Cleanup
    free(res);
	free(l3i);
	l3_unmonitorall(l3);
    l3_release(l3);
    return 0;
}




*/