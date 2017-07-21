//
// Created by barkerm on 14/04/17.
//

#ifndef PACKET_MMAP_LATENCY_HANDLER_H
#define PACKET_MMAP_LATENCY_HANDLER_H

#include "common.h"

void vmars_latency_sighandler(int num);

typedef struct
{
    vmars_spsc_rb_t** ring_buffers;
    int len;
} vmars_buffer_vec_t;

typedef struct
{
    vmars_config_t* config;
    vmars_buffer_vec_t buffer_vec;
    struct jodie* jodie_server;
} vmars_latency_handler_context_t;

typedef struct
{
    int msg_type;

    int64_t tv_sec;
    int64_t tv_nsec;

    char key[0];
} latency_record_t;

void* poll_ring_buffers(void* context);

#endif //PACKET_MMAP_LATENCY_HANDLER_H
