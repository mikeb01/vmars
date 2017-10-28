#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/prctl.h>

#include "ringbuffer.h"
#include "common.h"
#include "counter_handler.h"
#include "packet.h"
#include "jodie_client.h"

void root_sighandler(int num)
{
    printf("Terminating\n");

    vmars_packet_sighandler(num);
    vmars_counters_sighandler(num);
}

const char* USAGE =
    "VMars FIX packet probe \n"
    "Usage: vmars [OPTIONS] \n"
    "\n"
    "Options:\n"
    "  -i  --interface name[,name]     A list of interfaces to probe for messages.\n"
    "                                  E.g. -i eth0,eth1.\n"
    "  -t  --num-threads num           Number of polling threads per interface (default 1).\n"
    "  -a  --affinity num[,num]        A list of CPUs to use for networking threads.\n"
    "                                  E.g. -a 0,2.  The CPU assignments match respectively\n"
    "                                  with list of interfaces.  The first 'num-threads' cores\n"
    "                                  are assigned to the first interface and so on...\n"
    "  -c  --capture-port num          The port to capture messages on.\n"
    "  -h  --udp-host ip-string        IP address to send latency updates to.\n"
    "  -p  --udp-host num              UDP port to send lantency updates to.\n"
    "  -H  --hardware-timestamps       Enable hardware packet timestamps, will fall back to default\n"
    "                                  if the hardware does not support it.\n"
    "\n";

static struct option long_options[] =
{
    { "interface", required_argument, NULL, 'i' },
    { "num-threads", required_argument, NULL, 't' },
    { "affinity", required_argument, NULL, 'a' },
    { "capture-port", required_argument, NULL, 'c' },
    { "udp-host", required_argument, NULL, 'h' },
    { "udp-port", required_argument, NULL, 'p' },
    { "hardware-timestamps", no_argument, NULL, 'H' },
    { 0, 0, 0, 0 }
};

static void set_default_config(vmars_config_t* config)
{
    memset(config, 0, sizeof(vmars_config_t));
    config->affinity = "";
}

static vmars_str_vec_t split_string(const char* s)
{
    vmars_str_vec_t vec = {};

    char* dup_s = strdup(s);

    int count = 0;

    char* token = strtok(dup_s, ",");
    while (NULL != token)
    {
        count++;
        token = strtok(NULL, ",");
    }

    vec.strings = (const char**) malloc(count * sizeof(char*));
    vec.len = count;

    free(dup_s);

    dup_s = strdup(s);

    int i = 0;
    token = strtok(dup_s, ",");
    while (NULL != token)
    {
        vec.strings[i] = token;
        token = strtok(NULL, ",");
        i++;
    }

    return vec;
}

int parse_cpu_id(const char* s)
{
    prctl(PR_SET_DUMPABLE, 1);

    char* endptr = NULL;
    long int cpu_id = strtol(s, &endptr, 10);

    if (endptr[0] != '\0')
    {
        fprintf(stderr, "Skipping affinity setting of: %s\n", s);
        cpu_id = -1;
    }

    return (int) cpu_id;
}

int main(int argc, char** argp)
{
    int opt;
    vmars_config_t config = {};
    vmars_counters_context_t counters_context = {};
    struct jodie jodie_for_monitoring = {};

    set_default_config(&config);

    int option_index = 0;
    while ((opt = getopt_long(argc, argp, "i:t:a:c:h:p:H", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'i':
                config.interfaces = optarg;
                break;

            case 't':
                config.num_threads = atoi(optarg);
                break;

            case 'a':
                config.affinity = optarg;

            case 'c':
                config.capture_port = atoi(optarg);
                break;

            case 'h':
                config.udp_host = optarg;
                break;

            case 'p':
                config.udp_port = atoi(optarg);
                break;

            case 'H':
                config.use_hw_timestamps = 1;
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

    vmars_ringbuffer_ctx aeron_ctx = vmars_ringbufer_setup();

    vmars_verbose(
        "Interfaces: %s, port: %d, num threads: %d, affinity: %s, udp host: %s, udp port: %d\n",
        config.interfaces, config.capture_port, config.num_threads, config.affinity, config.udp_host, config.udp_port);
    
    const vmars_str_vec_t interfaces = split_string(config.interfaces);
    const vmars_str_vec_t affinity = split_string(config.affinity);

    int total_threads = interfaces.len * config.num_threads;

    pthread_t* polling_threads = (pthread_t*) calloc((size_t) total_threads, sizeof(pthread_t));
    pthread_t* counters_thread = (pthread_t*) calloc(1, sizeof(pthread_t));

    vmars_capture_context_t* thread_contexts =
        (vmars_capture_context_t*) calloc((size_t) total_threads, sizeof(vmars_capture_context_t));

    vmars_monitoring_counters_vec_t counters_vec = {};
    counters_vec.counters =
        (vmars_monitoring_counters_t**) calloc((size_t) total_threads, sizeof(vmars_monitoring_counters_t*));
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

            int cpu_id = (i < affinity.len) ? parse_cpu_id(affinity.strings[i]) : -1;

            thread_contexts[idx].config = &config;
            thread_contexts[idx].thread_num = idx;
            thread_contexts[idx].fanout_id = fanout_id * (j + 1);
            thread_contexts[idx].interface = interface;
            thread_contexts[idx].port = config.capture_port;
            thread_contexts[idx].cpu_num = cpu_id;
            thread_contexts[idx].aeron_ctx = aeron_ctx;

            counters_vec.counters[idx] = &thread_contexts[idx].counters;

            pthread_create(&polling_threads[idx], NULL, vmars_poll_socket, &thread_contexts[idx]);
        }
    }

    jodie_init(config.udp_host, config.udp_port, &jodie_for_monitoring);

    counters_context.counters_vec = counters_vec;
    counters_context.jodie_server = &jodie_for_monitoring;

    pthread_create(counters_thread, NULL, vmars_poll_counters, &counters_context);

    for (int i = 0; i < config.num_threads; i++)
    {
        pthread_join(polling_threads[i], NULL);
    }

//    pthread_join(*counters_thread, NULL);

    jodie_close(&jodie_for_monitoring);

    return 0;
}

