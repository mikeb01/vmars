#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include "packet.h"

int main(int argc, char** argp)
{
    int opt;
    char* interface = NULL;
    int num_threads = 2;
    int port = -1;

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
    capture_context_t* thread_contexts = calloc((size_t) num_threads, sizeof(capture_context_t));

    signal(SIGINT, sighandler);
    int fanout_id = 1111;

    for (int i = 0; i < num_threads; i++)
    {
        thread_contexts[i].thread_num = i;
        thread_contexts[i].fanout_id = fanout_id;
        thread_contexts[i].interface = argp[1];
        thread_contexts[i].port = port;

        pthread_create(&polling_threads[i], NULL, poll_socket, &thread_contexts[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(polling_threads[i], NULL);
    }

    return 0;
}

