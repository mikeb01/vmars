//
// Created by barkerm on 15/04/17.
//

#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#include "counter_handler.h"

static sig_atomic_t sigint = 0;

void counters_sighandler(int num)
{
    sigint = 1;
}

void* poll_counters(void* context)
{
    monitoring_counters_vec_t* counters_vec = (monitoring_counters_vec_t*) context;
    monitoring_counters_t aggregate_counters = { .corrupt_messages = 0, .invalid_checksums = 0 };

    while (!sigint)
    {
        sleep(10);

        for (int i = 0; i < counters_vec->len; i++)
        {
            aggregate_counters.invalid_checksums += counters_vec->counters[i]->invalid_checksums;
            aggregate_counters.corrupt_messages += counters_vec->counters[i]->corrupt_messages;
        }

        printf(
            "Invalid checksums: %d, corrupt messages: %d\n",
            aggregate_counters.invalid_checksums,
            aggregate_counters.corrupt_messages);
    }

    pthread_exit(NULL);
}