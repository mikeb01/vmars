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
    uint32_t saddr,
    uint16_t sport,
    uint32_t daddr,
    uint16_t dport,
    unsigned int tv_sec,
    unsigned int tv_nsec,
    const char* data_ptr,
    size_t data_len);

#endif //PACKET_MMAP_FIX_H
