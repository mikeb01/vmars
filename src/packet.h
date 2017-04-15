//
// Created by barkerm on 4/04/17.
//

#ifndef PACKET_MMAP_PACKET_H
#define PACKET_MMAP_PACKET_H

#include "counter_handler.h"

typedef struct
{
    int thread_num;
    int fanout_id;
    char* interface;
    int port;
    struct boyermoore_s* matcher;
    spsc_rb_t* rb;
    monitoring_counters_t counters;
} capture_context_t;

void packet_sighandler(int num);

void* poll_socket(void* context);

#endif //PACKET_MMAP_PACKET_H
