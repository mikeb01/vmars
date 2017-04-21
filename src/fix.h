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
} vmars_fix_message_summary_t;

void vmars_extract_fix_messages(
    vmars_capture_context_t* ctx,
    uint32_t tv_sec, uint32_t tv_nsec,
    const char* data_ptr, size_t data_len);

#endif //PACKET_MMAP_FIX_H
