//
// Created by barkerm on 4/04/17.
//

#ifndef PACKET_MMAP_PACKET_H
#define PACKET_MMAP_PACKET_H

typedef struct
{
    int thread_num;
    int fanout_id;
    char* interface;
    int port;
    struct boyermoore_s* matcher;
    spsc_rb_t* rb;
    vmars_monitoring_counters_t counters;
} capture_context_t;

void packet_sighandler(int num);

void* poll_socket(void* context);

#endif //PACKET_MMAP_PACKET_H
