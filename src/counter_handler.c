//
// Created by barkerm on 15/04/17.
//

#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "counter_handler.h"

static sig_atomic_t sigint = 0;

void vmars_counters_sighandler(int num)
{
    sigint = 1;
}

void* vmars_poll_counters(void* context)
{
    vmars_counters_context_t* ctx = (vmars_counters_context_t*) context;
    vmars_monitoring_counters_t aggregate_counters;

    while (!sigint)
    {
        sleep(2);
        memset(&aggregate_counters, 0, sizeof(vmars_monitoring_counters_t));

        for (int i = 0; i < ctx->counters_vec.len; i++)
        {
            aggregate_counters.valid_messages += ctx->counters_vec.counters[i]->valid_messages;
            aggregate_counters.invalid_checksums += ctx->counters_vec.counters[i]->invalid_checksums;
            aggregate_counters.corrupt_messages += ctx->counters_vec.counters[i]->corrupt_messages;
        }

        printf(
            "Valid fix messages: %ld, invalid checksums: %ld, corrupt messages: %ld\n",
            aggregate_counters.valid_messages,
            aggregate_counters.invalid_checksums,
            aggregate_counters.corrupt_messages);
    }

    pthread_exit(NULL);
}