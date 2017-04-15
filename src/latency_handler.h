//
// Created by barkerm on 14/04/17.
//

#ifndef PACKET_MMAP_LATENCY_HANDLER_H
#define PACKET_MMAP_LATENCY_HANDLER_H

void latency_sighandler(int num);

typedef struct
{
    spsc_rb_t** ring_buffers;
    int len;
} buffer_vec_t;

void* poll_ring_buffers(void* context);

#endif //PACKET_MMAP_LATENCY_HANDLER_H
