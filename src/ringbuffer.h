//
// Created by barkerm on 26/10/17.
//

#ifndef VMARS_AERON_SENDER_H

typedef void* vmars_ringbuffer_ctx;

vmars_ringbuffer_ctx vmars_ringbufer_setup();

int64_t vmars_ringbuffer_send(
    vmars_ringbuffer_ctx sender,
    uint32_t tp_sec,
    uint32_t tp_nsec,
    uint8_t* buffer,
    int32_t length);

#define VMARS_AERON_SENDER_H

#endif //VMARS_AERON_SENDER_H
