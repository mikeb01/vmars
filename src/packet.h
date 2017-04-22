//
// Created by barkerm on 4/04/17.
//

#ifndef PACKET_MMAP_PACKET_H
#define PACKET_MMAP_PACKET_H

typedef struct
{
    int thread_num;
    int fanout_id;
    const char* interface;
    int port;
    struct boyermoore_s* matcher;
    vmars_spsc_rb_t* rb;
    vmars_monitoring_counters_t counters;
} vmars_capture_context_t;

void vmars_packet_sighandler(int num);

void* vmars_poll_socket(void* context);

#endif //PACKET_MMAP_PACKET_H
