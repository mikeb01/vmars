//
// Created by barkerm on 9/04/17.
//

#ifndef PACKET_MMAP_SPSC_H
#define PACKET_MMAP_SPSC_H

#define CACHE_LINE_LENGTH (64)

#pragma pack(push)
#pragma pack(4)
typedef struct
{
    uint8_t begin_pad[(2 * CACHE_LINE_LENGTH)];

    int64_t tail_cache_position;
    uint8_t tail_cache_pad[(2 * CACHE_LINE_LENGTH) - sizeof(int64_t)];

    int64_t tail_position;
    uint8_t tail_pad[(2 * CACHE_LINE_LENGTH) - sizeof(int64_t)];

    int64_t head_cache_position;
    uint8_t head_cache_pad[(2 * CACHE_LINE_LENGTH) - sizeof(int64_t)];

    int64_t head_position;
    uint8_t head_pad[(2 * CACHE_LINE_LENGTH) - sizeof(int64_t)];

} rb_descriptor_t;

typedef struct
{
    int32_t length;
    uint8_t data[0];
} rb_record_t;
#pragma pack(pop)


typedef struct
{
    uint8_t* buffer;
    rb_descriptor_t* descriptor;
    size_t capacity;
    size_t max_message_length;
} spsc_rb_t;


int spsc_rb_init(spsc_rb_t* rb, size_t capacity);

rb_record_t* spsc_rb_claim(spsc_rb_t* rb, size_t length);

int spsc_rb_publish(spsc_rb_t* rb, rb_record_t* to_publish);

const rb_record_t* spsc_rb_poll(spsc_rb_t* rb);

#endif //PACKET_MMAP_SPSC_H
