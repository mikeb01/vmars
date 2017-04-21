//
// Created by barkerm on 21/04/17.
//

#ifndef PACKET_MMAP_COMMON_H
#define PACKET_MMAP_COMMON_H

typedef struct
{
    char* udp_host;
    int udp_port;
    int capture_port;
    char* interface;
    int num_threads;
} vmars_config_t;

#endif //PACKET_MMAP_COMMON_H
