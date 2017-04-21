#define _GNU_SOURCE

#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <hdr_histogram.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "common.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "fix.h"
#include "fixparser.h"
#include "latency_handler.h"
#include "khash.h"
#include "JodieClient.h"

#define MAX_POLLS 20

static sig_atomic_t sigint = 0;

void vmars_latency_sighandler(int num)
{
    sigint = 1;
}

typedef struct
{
    int msg_type;

    int64_t tv_sec;
    int64_t tv_nsec;

    char key[0];
} latency_record_t;

KHASH_MAP_INIT_STR(latency, latency_record_t*);

static bool is_begin(int type)
{
    switch (type)
    {
        case MSG_TYPE_MASS_QUOTE:
        case MSG_TYPE_NEW_ORDER_SINGLE:
        case MSG_TYPE_TRACE_REQ:
            return true;

        default:
            return false;
    }
}

static int64_t diff(int64_t t0_sec, int64_t t0_nsec, int64_t t1_sec, int64_t t1_nsec)
{
    int64_t diff_sec = t1_sec - t0_sec;
    int64_t diff_nsec = t1_nsec - t0_nsec;

    return (diff_sec * 1000000000) + diff_nsec;
}

static bool is_complement(int type_a, int type_b)
{
    switch (type_a)
    {
        case MSG_TYPE_MASS_QUOTE:
            return type_b == MSG_TYPE_MASS_QUOTE_ACK;
        case MSG_TYPE_MASS_QUOTE_ACK:
            return type_a == MSG_TYPE_MASS_QUOTE;
        case MSG_TYPE_NEW_ORDER_SINGLE:
            return MSG_TYPE_EXECUTION_REPORT;
        case MSG_TYPE_EXECUTION_REPORT:
            return MSG_TYPE_NEW_ORDER_SINGLE;
        case MSG_TYPE_TRACE_REQ:
            return MSG_TYPE_TRACE_RSP;
        case MSG_TYPE_TRACE_RSP:
            return MSG_TYPE_TRACE_REQ;

        default:
            return false;
    }
}

void calculate_latency(kh_latency_t* latency_table, struct hdr_histogram* histogram, vmars_fix_message_summary_t* msg)
{
    int ret;
    khint_t k;

    k = kh_get(latency, latency_table, msg->key);
    if (k != kh_end(latency_table))
    {
        latency_record_t* latency_record = kh_val(latency_table, k);

        if (NULL != latency_record && is_complement(msg->msg_type, latency_record->msg_type))
        {
            int64_t latency_nsec;
            if (is_begin(msg->msg_type))
            {
                latency_nsec = diff(msg->tv_sec, msg->tv_nsec, latency_record->tv_sec, latency_record->tv_nsec);
            }
            else
            {
                latency_nsec = diff(latency_record->tv_sec, latency_record->tv_nsec, msg->tv_sec, msg->tv_nsec);
            }

            kh_val(latency_table, k) = NULL;
            kh_del(latency, latency_table, k);
            free(latency_record);

            hdr_record_value(histogram, latency_nsec);

            printf("Found, latency = %ld\n", latency_nsec);
        }
    }
    else
    {
        latency_record_t* latency_record = (latency_record_t*) malloc(sizeof(latency_record_t) + (size_t) (msg->key_len + 1));
        strncpy(latency_record->key, msg->key, (size_t) msg->key_len);
        latency_record->key[msg->key_len] = '\0';
        latency_record->msg_type = msg->msg_type;

        latency_record->tv_sec = msg->tv_sec;
        latency_record->tv_nsec = msg->tv_nsec;

        k = kh_put(latency, latency_table, latency_record->key, &ret);
        kh_val(latency_table, k) = latency_record;
    }
}

void* poll_ring_buffers(void* context)
{
    vmars_latency_handler_context_t* ctx = (vmars_latency_handler_context_t*) context;
    khash_t(latency)* latency_table = kh_init(latency);
    struct timespec last_timestamp;
    struct timespec curr_timestamp;
    struct hdr_histogram* histogram;
    hdr_init(1, INT64_C(10000000000), 2, &histogram);

    jodie_server.init(ctx->config->udp_host, ctx->config->udp_port);

    clock_gettime(CLOCK_MONOTONIC, &last_timestamp);

    while (!sigint)
    {
        clock_gettime(CLOCK_MONOTONIC, &curr_timestamp);

        if (curr_timestamp.tv_sec > last_timestamp.tv_sec)
        {
            printf("Max latency: %ld\n", hdr_max(histogram));
            jodie_server.logGauge("vmars.latency.max", hdr_max(histogram), curr_timestamp.tv_sec * 1000);
            jodie_server.flush();

            last_timestamp = curr_timestamp;
            hdr_reset(histogram);
        }

        for (int i = 0; i < ctx->buffer_vec.len; i++)
        {
            vmars_spsc_rb_t* rb = ctx->buffer_vec.ring_buffers[i];
            int poll_limit = MAX_POLLS;

            const vmars_rb_record_t* record;
            while (NULL != (record = vmars_spsc_rb_poll(rb)) && --poll_limit > 0)
            {
                vmars_fix_message_summary_t* msg = (vmars_fix_message_summary_t*) record->data;

                calculate_latency(latency_table, histogram, msg);

                vmars_spsc_rb_release(rb, record);
            }
        }
    }

    pthread_exit(NULL);
}
