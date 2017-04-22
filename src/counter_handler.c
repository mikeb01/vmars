//
// Created by barkerm on 15/04/17.
//
#define _GNU_SOURCE

#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/if_packet.h>
#include <sys/socket.h>

#include "common.h"
#include "counter_handler.h"

static sig_atomic_t sigint = 0;

void vmars_counters_sighandler(int num)
{
    sigint = 1;
}

void gather_socket_stats(int fd, struct tpacket_stats_v3* aggregate_stats)
{
    if (fd == 0)
    {
        return;
    }

    struct tpacket_stats_v3 stats;
    socklen_t len = sizeof(stats);
    int err = getsockopt(fd, SOL_PACKET, PACKET_STATISTICS, &stats, &len);
    if (err < 0)
    {
        return;
    }

    aggregate_stats->tp_drops += stats.tp_drops;
    aggregate_stats->tp_packets += stats.tp_packets;
    aggregate_stats->tp_freeze_q_cnt += stats.tp_freeze_q_cnt;
}


void* vmars_poll_counters(void* context)
{
    vmars_counters_context_t* ctx = (vmars_counters_context_t*) context;
    vmars_monitoring_counters_t aggregate_counters;
    struct tpacket_stats_v3 stats;

    while (!sigint)
    {
        sleep(2);
        memset(&aggregate_counters, 0, sizeof(vmars_monitoring_counters_t));
        memset(&stats, 0, sizeof(struct tpacket_stats_v3));


        for (int i = 0; i < ctx->counters_vec.len; i++)
        {
            aggregate_counters.valid_messages += ctx->counters_vec.counters[i]->valid_messages;
            aggregate_counters.invalid_checksums += ctx->counters_vec.counters[i]->invalid_checksums;
            aggregate_counters.corrupt_messages += ctx->counters_vec.counters[i]->corrupt_messages;
            aggregate_counters.bytes_total += ctx->counters_vec.counters[i]->bytes_total;

            gather_socket_stats(ctx->counters_vec.counters[i]->fd, &stats);
        }

        printf(
            "Valid fix messages: %ld, invalid checksums: %ld, corrupt messages: %ld\n",
            aggregate_counters.valid_messages,
            aggregate_counters.invalid_checksums,
            aggregate_counters.corrupt_messages);

        printf(
            "Bytes: %ld, packets: %ld, drops: %ld, freeze-q-cnt: %ld\n",
            aggregate_counters.bytes_total,
            stats.tp_packets,
            stats.tp_drops,
            stats.tp_freeze_q_cnt);
    }

    pthread_exit(NULL);
}