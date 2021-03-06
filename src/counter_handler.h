//
// Created by barkerm on 15/04/17.
//

#ifndef PACKET_MMAP_COUNTER_HANDLER_H
#define PACKET_MMAP_COUNTER_HANDLER_H

typedef struct
{
    int fd;
    int64_t invalid_checksums;
    int64_t corrupt_messages;
    int64_t valid_messages;
    int64_t parsing_errors;
    int64_t bytes_total;
    int64_t mass_quote_count;
    int64_t mass_quote_ack_count;
    int64_t new_order_single_count;
    int64_t execution_report_new_count;
    int64_t trace_request_count;
    int64_t trace_repsonse_count;
} vmars_monitoring_counters_t;

typedef struct
{
    vmars_monitoring_counters_t** counters;
    int len;
} vmars_monitoring_counters_vec_t;

typedef struct
{
    vmars_monitoring_counters_vec_t counters_vec;
    vmars_config_t config;
    struct jodie* jodie_server;
} vmars_counters_context_t;

void vmars_counters_sighandler(int num);

void* vmars_poll_counters(void* context);

#endif //PACKET_MMAP_COUNTER_HANDLER_H
