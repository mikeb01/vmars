//
// Created by barkerm on 26/10/17.
//

#ifndef VMARS_AERON_SENDER_H

typedef void* vmars_aeron_ctx;

vmars_aeron_ctx vmars_aeron_setup();

int64_t vmars_aeron_send(vmars_aeron_ctx sender, uint8_t* buffer, int32_t length);

#define VMARS_AERON_SENDER_H

#endif //VMARS_AERON_SENDER_H
