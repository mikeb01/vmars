//
// Created by barkerm on 15/04/17.
//

#ifndef PACKET_MMAP_COUNTER_HANDLER_H
#define PACKET_MMAP_COUNTER_HANDLER_H

typedef struct
{
    int64_t invalid_checksums;
    int64_t corrupt_messages;
} monitoring_counters_t;

typedef struct
{
    monitoring_counters_t** counters;
    int len;
} monitoring_counters_vec_t;

void counters_sighandler(int num);

void* poll_counters(void* context);

#endif //PACKET_MMAP_COUNTER_HANDLER_H
