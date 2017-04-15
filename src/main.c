#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>

#include "spsc_rb.h"
#include "packet.h"
#include "latency_handler.h"
#include "counter_handler.h"

void root_sighandler(int num)
{
    printf("Terminating\n");

    packet_sighandler(num);
    latency_sighandler(num);
    counters_sighandler(num);
}

int main(int argc, char** argp)
{
    int opt;
    char* interface = NULL;
    int num_threads = 2;
    int port = -1;
    buffer_vec_t buffer_vec;
    monitoring_counters_vec_t counters_vec;

    while ((opt = getopt(argc, argp, "i:t:p:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                interface = optarg;
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n", argp[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (num_threads < 1)
    {
        fprintf(stderr, "Number of threads (-n) must be positive\n");
        exit(EXIT_FAILURE);
    }
    if (NULL == interface)
    {
        fprintf(stderr, "Must specify an interface (-i)\n");
        exit(EXIT_FAILURE);
    }
    if (-1 == port)
    {
        fprintf(stderr, "Must specify a port (-p)\n");
        exit(EXIT_FAILURE);
    }

    pthread_t* polling_threads = calloc((size_t) num_threads, sizeof(pthread_t));
    pthread_t* latency_thread = calloc(1, sizeof(pthread_t));
    pthread_t* counters_thread = calloc(1, sizeof(pthread_t));

    capture_context_t* thread_contexts = calloc((size_t) num_threads, sizeof(capture_context_t));

    buffer_vec.ring_buffers = calloc((size_t) num_threads, sizeof(spsc_rb_t*));
    buffer_vec.len = num_threads;

    counters_vec.counters = calloc((size_t) num_threads, sizeof(spsc_rb_t*));
    counters_vec.len = num_threads;

    signal(SIGINT, root_sighandler);
    int fanout_id = 1111;

    for (int i = 0; i < num_threads; i++)
    {
        thread_contexts[i].thread_num = i;
        thread_contexts[i].fanout_id = fanout_id;
        thread_contexts[i].interface = argp[1];
        thread_contexts[i].port = port;
        thread_contexts[i].rb = (spsc_rb_t*) calloc(1, sizeof(spsc_rb_t));

        if (thread_contexts[i].rb == NULL)
        {
            fprintf(stderr, "Failed to allocate ring buffer.\n");
            exit(EXIT_FAILURE);
        }

        if (spsc_rb_init(thread_contexts[i].rb, 4096) < 0)
        {
            fprintf(stderr, "Failed to init ring buffer.\n");
            exit(EXIT_FAILURE);
        }

        buffer_vec.ring_buffers[i] = thread_contexts[i].rb;
        counters_vec.counters[i] = &thread_contexts[i].counters;

        pthread_create(&polling_threads[i], NULL, poll_socket, &thread_contexts[i]);
    }

    pthread_create(latency_thread, NULL, poll_ring_buffers, &buffer_vec);
    pthread_create(counters_thread, NULL, poll_counters, &counters_vec);

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(polling_threads[i], NULL);
    }

    pthread_join(*latency_thread, NULL);
    pthread_join(*counters_thread, NULL);

    return 0;
}

