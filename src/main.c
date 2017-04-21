#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "latency_handler.h"

void root_sighandler(int num)
{
    printf("Terminating\n");

    vmars_packet_sighandler(num);
    vmars_latency_sighandler(num);
    vmars_counters_sighandler(num);
}

static struct option long_options[] =
{
    { "interface", required_argument, NULL, 'i' },
    { "num-threads", required_argument, NULL, 't' },
    { "capture-port", required_argument, NULL, 'c' },
    { "udp-host", required_argument, NULL, 'h' },
    { "udp-port", required_argument, NULL, 'p' },
    { 0, 0, 0, 0}
};

int main(int argc, char** argp)
{
    int opt;
    vmars_config_t config;
    vmars_latency_handler_context_t latency_context;
    vmars_monitoring_counters_vec_t counters_vec;

    int option_index = 0;
    while ((opt = getopt_long(argc, argp, "i:t:c:h:p:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'i':
                config.interface = optarg;
                break;
            case 't':
                config.num_threads = atoi(optarg);
                break;
            case 'c':
                config.capture_port = atoi(optarg);
                break;

            case 'h':
                config.udp_host = optarg;
                break;

            case 'p':
                config.udp_port = atoi(optarg);
                break;

            default: /* '?' */
                fprintf(stderr, "Usage: %s [-t nsecs] [-n ] name\n", argp[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config.num_threads < 1)
    {
        fprintf(stderr, "Number of threads (-n) must be positive\n");
        exit(EXIT_FAILURE);
    }
    if (NULL == config.interface)
    {
        fprintf(stderr, "Must specify an interface (-i)\n");
        exit(EXIT_FAILURE);
    }
    if (-1 == config.capture_port)
    {
        fprintf(stderr, "Must specify a port (-p)\n");
        exit(EXIT_FAILURE);
    }

    printf(
        "Interface: %s, port: %d, num threads: %d, udp host: %s, udp port: %d\n",
        config.interface, config.capture_port, config.num_threads, config.udp_host, config.udp_port);

    pthread_t* polling_threads = calloc((size_t) config.num_threads, sizeof(pthread_t));
    pthread_t* latency_thread = calloc(1, sizeof(pthread_t));
    pthread_t* counters_thread = calloc(1, sizeof(pthread_t));

    vmars_capture_context_t* thread_contexts = calloc((size_t) config.num_threads, sizeof(vmars_capture_context_t));

    latency_context.config = &config;
    latency_context.buffer_vec.ring_buffers = calloc((size_t) config.num_threads, sizeof(vmars_spsc_rb_t*));
    latency_context.buffer_vec.len = config.num_threads;

    counters_vec.counters = calloc((size_t) config.num_threads, sizeof(vmars_spsc_rb_t*));
    counters_vec.len = config.num_threads;

    signal(SIGINT, root_sighandler);
    int fanout_id = 1111;

    for (int i = 0; i < config.num_threads; i++)
    {
        thread_contexts[i].thread_num = i;
        thread_contexts[i].fanout_id = fanout_id;
        thread_contexts[i].interface = argp[1];
        thread_contexts[i].port = config.capture_port;
        thread_contexts[i].rb = (vmars_spsc_rb_t*) calloc(1, sizeof(vmars_spsc_rb_t));

        if (thread_contexts[i].rb == NULL)
        {
            fprintf(stderr, "Failed to allocate ring buffer.\n");
            exit(EXIT_FAILURE);
        }

        if (vmars_spsc_rb_init(thread_contexts[i].rb, 4096) < 0)
        {
            fprintf(stderr, "Failed to init ring buffer.\n");
            exit(EXIT_FAILURE);
        }

        latency_context.buffer_vec.ring_buffers[i] = thread_contexts[i].rb;
        counters_vec.counters[i] = &thread_contexts[i].counters;

        pthread_create(&polling_threads[i], NULL, vmars_poll_socket, &thread_contexts[i]);
    }

    pthread_create(latency_thread, NULL, poll_ring_buffers, &latency_context);
    pthread_create(counters_thread, NULL, vmars_poll_counters, &counters_vec);

    for (int i = 0; i < config.num_threads; i++)
    {
        pthread_join(polling_threads[i], NULL);
    }

    pthread_join(*latency_thread, NULL);
    pthread_join(*counters_thread, NULL);

    return 0;
}

