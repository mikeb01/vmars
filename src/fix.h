//
// Created by barkerm on 13/04/17.
//

#ifndef PACKET_MMAP_FIX_H
#define PACKET_MMAP_FIX_H

typedef struct
{
    int64_t tv_sec;
    int64_t tv_nsec;
    int msg_type;
    int32_t key_len;
    char key[0];
} fix_message_summary_t;

void extract_fix_messages(capture_context_t* ctx, const char* data_ptr, size_t data_len);

#endif //PACKET_MMAP_FIX_H
