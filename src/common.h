//
// Created by barkerm on 21/04/17.
//

#ifndef PACKET_MMAP_COMMON_H
#define PACKET_MMAP_COMMON_H

typedef struct
{
    const char** strings;
    int len;
} vmars_str_vec_t;

typedef struct
{
    char* udp_host;
    int udp_port;
    int capture_port;
    char* interfaces;
    int num_threads;
} vmars_config_t;

#endif //PACKET_MMAP_COMMON_H
