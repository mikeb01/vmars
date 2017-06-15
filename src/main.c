#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <netinet/in.h>

#include "common.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "latency_handler.h"
#include "JodieClient.h"

void root_sighandler(int num)
{
    printf("Terminating\n");

    vmars_packet_sighandler(num);
    vmars_latency_sighandler(num);
    vmars_counters_sighandler(num);
}

const char* USAGE =
    "VMars FIX packet probe \n"
    "Usage: vmars [OPTIONS] \n"
    "\n"
    "Options:\n"
    "  -i  --interface name[,name]     A list of interfaces to probe for messages.\n"
    "                                  E.g. -i eth0,eth1.\n"
    "  -t  --num-threads num           Number of polling threads per interface.\n"
    "  -c  --capture-port num          The port to capture messages on.\n"
    "  -h  --udp-host ip-string        IP address to send latency updates to.\n"
    "  -p  --udp-host num              UDP port to send lantency updates to.\n"
    "\n";

static struct option long_options[] =
{
    { "interface", required_argument, NULL, 'i' },
    { "num-threads", required_argument, NULL, 't' },
    { "capture-port", required_argument, NULL, 'c' },
    { "udp-host", required_argument, NULL, 'h' },
    { "udp-port", required_argument, NULL, 'p' },
    { 0, 0, 0, 0 }
};

static vmars_str_vec_t split_interfaces(const char* interfaces)
{
    vmars_str_vec_t vec;

    char* dup_ifaces = strdup(interfaces);

    int count = 0;

    char* token = strtok(dup_ifaces, ",");
    while (NULL != token)
    {
        count++;
        token = strtok(NULL, ",");
    }

    vec.strings = malloc(count * sizeof(char*));
    vec.len = count;

    free(dup_ifaces);

    dup_ifaces = strdup(interfaces);

    int i = 0;
    token = strtok(dup_ifaces, ",");
    while (NULL != token)
    {
        vec.strings[i] = token;
        token = strtok(NULL, ",");
        i++;
    }

    return vec;
}

int main(int argc, char** argp)
{
    int opt;
    vmars_config_t config;
    vmars_latency_handler_context_t latency_context;
    vmars_counters_context_t counters_context;
    struct jodie jodie_for_latency;
    struct jodie jodie_for_monitoring;

    memset(&config, 0, sizeof(vmars_config_t));

    int option_index = 0;
    while ((opt = getopt_long(argc, argp, "i:t:c:h:p:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'i':
                config.interfaces = optarg;
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
                fprintf(stderr, "%s", USAGE);
                exit(EXIT_FAILURE);
        }
    }

    if (config.num_threads < 1)
    {
        fprintf(stderr, "Number of threads must be positive\n");
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    }
    if (NULL == config.interfaces)
    {
        fprintf(stderr, "Must specify an interface or interfaces\n");
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    }
    if (-1 == config.capture_port)
    {
        fprintf(stderr, "Must specify a capture port\n");
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    }
    if (NULL == config.udp_host)
    {
        fprintf(stderr, "Must specify a udp host\n");
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    }
    if (0 == config.udp_port)
    {
        fprintf(stderr, "Must specify a udp port\n");
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    }

    printf(
        "Interfaces: %s, port: %d, num threads: %d, udp host: %s, udp port: %d\n",
        config.interfaces, config.capture_port, config.num_threads, config.udp_host, config.udp_port);

    const vmars_str_vec_t interfaces = split_interfaces(config.interfaces);
    int total_threads = interfaces.len * config.num_threads;

    pthread_t* polling_threads = calloc((size_t) total_threads, sizeof(pthread_t));
    pthread_t* latency_thread = calloc(1, sizeof(pthread_t));
    pthread_t* counters_thread = calloc(1, sizeof(pthread_t));

    vmars_capture_context_t* thread_contexts = calloc((size_t) total_threads, sizeof(vmars_capture_context_t));

    latency_context.config = &config;
    latency_context.buffer_vec.ring_buffers = calloc((size_t) total_threads, sizeof(vmars_spsc_rb_t*));
    latency_context.buffer_vec.len = total_threads;

    vmars_monitoring_counters_vec_t counters_vec;
    counters_vec.counters = calloc((size_t) total_threads, sizeof(vmars_spsc_rb_t*));
    counters_vec.len = total_threads;

    signal(SIGINT, root_sighandler);
    int fanout_id = 1111;

    for (int j = 0; j < interfaces.len; j++)
    {
        const char* interface = interfaces.strings[j];

        for (int i = 0; i < config.num_threads; i++)
        {
            int base = j * config.num_threads;
            const int idx = base + i;

            thread_contexts[idx].thread_num = idx;
            thread_contexts[idx].fanout_id = fanout_id * (j + 1);
            thread_contexts[idx].interface = interface;
            thread_contexts[idx].port = config.capture_port;
            thread_contexts[idx].rb = (vmars_spsc_rb_t*) calloc(1, sizeof(vmars_spsc_rb_t));

            if (thread_contexts[idx].rb == NULL)
            {
                fprintf(stderr, "Failed to allocate ring buffer.\n");
                exit(EXIT_FAILURE);
            }

            if (vmars_spsc_rb_init(thread_contexts[idx].rb, 4096) < 0)
            {
                fprintf(stderr, "Failed to init ring buffer.\n");
                exit(EXIT_FAILURE);
            }

            latency_context.buffer_vec.ring_buffers[idx] = thread_contexts[idx].rb;
            counters_vec.counters[idx] = &thread_contexts[idx].counters;

            pthread_create(&polling_threads[idx], NULL, vmars_poll_socket, &thread_contexts[idx]);
        }
    }

    jodie_init(config.udp_host, config.udp_port, &jodie_for_latency);
    jodie_dup(&jodie_for_latency, &jodie_for_monitoring);

    counters_context.counters_vec = counters_vec;
    counters_context.jodie_server = &jodie_for_monitoring;

    latency_context.jodie_server = &jodie_for_latency;

    pthread_create(latency_thread, NULL, poll_ring_buffers, &latency_context);
    pthread_create(counters_thread, NULL, vmars_poll_counters, &counters_context);

    for (int i = 0; i < config.num_threads; i++)
    {
        pthread_join(polling_threads[i], NULL);
    }

    pthread_join(*latency_thread, NULL);
    pthread_join(*counters_thread, NULL);

    jodie_close(&jodie_for_latency);

    return 0;
}

