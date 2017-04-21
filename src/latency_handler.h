//
// Created by barkerm on 14/04/17.
//

#ifndef PACKET_MMAP_LATENCY_HANDLER_H
#define PACKET_MMAP_LATENCY_HANDLER_H

#include "common.h"

void latency_sighandler(int num);

typedef struct
{
    spsc_rb_t** ring_buffers;
    int len;
} buffer_vec_t;

typedef struct
{
    vmars_config_t* config;
    buffer_vec_t buffer_vec;
} vmars_latency_handler_context_t;

void* poll_ring_buffers(void* context);

#endif //PACKET_MMAP_LATENCY_HANDLER_H
