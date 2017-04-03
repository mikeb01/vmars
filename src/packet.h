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
} capture_context;

void sighandler(int num);

void extract_fix_messages(const capture_context* ctx, const char* data_ptr, size_t data_len);

void* poll_socket(void* context);

#endif //PACKET_MMAP_PACKET_H
