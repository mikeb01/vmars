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
} capture_context_t;

typedef struct
{
    int64_t tv_sec;
    int64_t tv_nsec;
    int msg_type;
    int32_t key_len;
    char key[0];
} fix_message_summary_t;

void sighandler(int num);

void extract_fix_messages(const capture_context_t* ctx, const char* data_ptr, size_t data_len);

void* poll_socket(void* context);

#endif //PACKET_MMAP_PACKET_H
