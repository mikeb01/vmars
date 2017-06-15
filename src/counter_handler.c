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
#include <netinet/in.h>

#include "common.h"
#include "counter_handler.h"
#include "JodieClient.h"

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
        sleep(1);
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

        const long timestamp = jodie_getMillis();
        jodie_logGauge(ctx->jodie_server, "vmars.valid_messages", aggregate_counters.valid_messages, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.invalid_checksums", aggregate_counters.valid_messages, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.corrupt_messages", aggregate_counters.valid_messages, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.network.bytes", aggregate_counters.bytes_total, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.network.packets", stats.tp_packets, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.network.drops", stats.tp_drops, timestamp);
        jodie_logGauge(ctx->jodie_server, "vmars.network.freeze_q_cnt", stats.tp_freeze_q_cnt, timestamp);
        jodie_flush(ctx->jodie_server);
    }

    pthread_exit(NULL);
}