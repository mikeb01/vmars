//
// Created by barkerm on 1/04/17.
//

/* Written from scratch, but kernel-to-user space API usage
 * dissected from lolpcap:
 *  Copyright 2011, Chetan Loke <loke.chetan@gmail.com>
 *  License: GPL, version 2.0
 */

// https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include "simpleboyermoore.h"
#include "spsc_rb.h"
#include "packet.h"
#include "fix.h"

#ifndef likely
# define likely(x)        __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
# define unlikely(x)        __builtin_expect(!!(x), 0)
#endif

struct ring
{
    struct iovec* rd;
    uint8_t* map;
    struct tpacket_req3 req;
    int fanout_id;
};


static unsigned long packets_total = 0, bytes_total = 0;
static sig_atomic_t sigint = 0;

struct block_desc
{
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
    uint32_t version;
    uint32_t offset_to_priv;
#pragma clang diagnostic pop
    struct tpacket_hdr_v1 h1;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void sighandler(int num)
{
    printf("Terminating\n");
    sigint = 1;
}
#pragma clang diagnostic pop

static int setup_socket(struct ring* ring, char* netdev)
{
    int err, i, fd, v = TPACKET_V3;
    struct sockaddr_ll ll;
    struct ifreq hwtstamp;
    struct hwtstamp_config hwconfig;
    unsigned int blocksiz = 1 << 22, framesiz = 1 << 11;
    unsigned int blocknum = 64;

    fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0)
    {
        perror("socket");
        return -1;
    }

    err = setsockopt(fd, SOL_PACKET, PACKET_VERSION, &v, sizeof(v));
    if (err < 0)
    {
        perror("setsockopt");
        return -1;
    }

    memset(&ring->req, 0, sizeof(ring->req));
    ring->req.tp_block_size = blocksiz;
    ring->req.tp_frame_size = framesiz;
    ring->req.tp_block_nr = blocknum;
    ring->req.tp_frame_nr = (blocksiz * blocknum) / framesiz;
    ring->req.tp_retire_blk_tov = 60;
    ring->req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

    err = setsockopt(fd, SOL_PACKET, PACKET_RX_RING, &ring->req, sizeof(ring->req));
    if (err < 0)
    {
        perror("setsockopt");
        return -1;
    }

    ring->map = mmap(
        NULL, ring->req.tp_block_size * ring->req.tp_block_nr, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
    if (ring->map == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }

    ring->rd = malloc(ring->req.tp_block_nr * sizeof(*ring->rd));
    for (i = 0; i < ring->req.tp_block_nr; ++i)
    {
        ring->rd[i].iov_base = ring->map + (i * ring->req.tp_block_size);
        ring->rd[i].iov_len = ring->req.tp_block_size;
    }

    memset(&ll, 0, sizeof(ll));
    ll.sll_family = PF_PACKET;
    ll.sll_protocol = htons(ETH_P_ALL);
    ll.sll_ifindex = if_nametoindex(netdev);
    ll.sll_hatype = 0;
    ll.sll_pkttype = 0;
    ll.sll_halen = 0;

    err = bind(fd, (struct sockaddr*) &ll, sizeof(ll));
    if (err < 0)
    {
        perror("Failed to bind to socket");
        return -1;
    }

    err = setsockopt(fd, SOL_PACKET, PACKET_FANOUT, &ring->fanout_id, sizeof(ring->fanout_id));
    if (err)
    {
        perror("Failed to set fanout option for socket");
        return -1;
    }

    memset(&hwtstamp, 0, sizeof(struct ifreq));
    memset(&hwconfig, 0, sizeof(struct hwtstamp_config));

    strncpy(hwtstamp.ifr_name, netdev, sizeof(hwtstamp.ifr_name));
    hwtstamp.ifr_data = (void *)&hwconfig;
    hwconfig.rx_filter = HWTSTAMP_FILTER_ALL;

    if (ioctl (fd, SIOCSHWTSTAMP, &hwtstamp) < 0)
    {
        perror("SIOCGHWTSTAMP failed - not using HW timestamps");
    }
    else
    {
        int req = SOF_TIMESTAMPING_RAW_HARDWARE;
        setsockopt(fd, SOL_PACKET, PACKET_TIMESTAMP, (void *) &req, sizeof(req));
    }

    return fd;
}

static void prettify_fix_message(char* msg, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (msg[i] == '\1')
        {
            msg[i] = '|';
        }
    }
}


static void display(capture_context_t* ctx, struct tpacket3_hdr* ppd)
{
    struct ethhdr* eth = (struct ethhdr*) ((uint8_t*) ppd + ppd->tp_mac);
    struct iphdr* ip = (struct iphdr*) ((uint8_t*) eth + ETH_HLEN);
    char data[256];

    if (eth->h_proto != htons(ETH_P_IP))
    {
        return;
    }

    int iphdr_len = ip->ihl * 4;
    struct tcphdr* tcp = (struct tcphdr*) ((uint8_t*) eth + ETH_HLEN + iphdr_len);
    uint16_t src_port = ntohs(tcp->source);
    uint16_t dst_port = ntohs(tcp->dest);

    if (src_port != ctx->port & dst_port != ctx->port)
    {
        return;
    }

    int doff = tcp->doff * 4;
    char* data_ptr = ((char*) tcp + doff);
    size_t data_len = (size_t) htons(ip->tot_len) - (iphdr_len + doff);

    if (data_len == 0)
    {
        return;
    }

    extract_fix_messages(ctx, data_ptr, data_len);

    size_t copy_len = data_len < 255 ? data_len : 255;

    strncpy(data, data_ptr, copy_len);
    data[copy_len] = '\0';
    prettify_fix_message(data, copy_len);

    struct sockaddr_in ss, sd;
    char sbuff[NI_MAXHOST], dbuff[NI_MAXHOST];

    memset(&ss, 0, sizeof(ss));
    ss.sin_family = PF_INET;
    ss.sin_addr.s_addr = ip->saddr;
    getnameinfo((struct sockaddr*) &ss, sizeof(ss),
                sbuff, sizeof(sbuff), NULL, 0, NI_NUMERICHOST);

    memset(&sd, 0, sizeof(sd));
    sd.sin_family = PF_INET;
    sd.sin_addr.s_addr = ip->daddr;
    getnameinfo((struct sockaddr*) &sd, sizeof(sd),
                dbuff, sizeof(dbuff), NULL, 0, NI_NUMERICHOST);

    printf(
        "[%d] %s:%d -> %s:%d, rxhash: 0x%x, %d:%09d, data: %s\n",
        ctx->thread_num, sbuff, src_port, dbuff, dst_port, ppd->hv1.tp_rxhash,
        ppd->tp_sec, ppd->tp_nsec, data);
}

static void walk_block(capture_context_t* ctx, struct block_desc* pbd)
{
    int num_pkts = pbd->h1.num_pkts, i;
    unsigned long bytes = 0;
    struct tpacket3_hdr* ppd;

    ppd = (struct tpacket3_hdr*) ((uint8_t*) pbd +
                                  pbd->h1.offset_to_first_pkt);
    for (i = 0; i < num_pkts; ++i)
    {
        bytes += ppd->tp_snaplen;
        display(ctx, ppd);

        ppd = (struct tpacket3_hdr*) ((uint8_t*) ppd + ppd->tp_next_offset);
    }

    packets_total += num_pkts;
    bytes_total += bytes;
}

static void flush_block(struct block_desc* pbd)
{
    pbd->h1.block_status = TP_STATUS_KERNEL;
}

static void teardown_socket(struct ring* ring, int fd)
{
    munmap(ring->map, ring->req.tp_block_size * ring->req.tp_block_nr);
    free(ring->rd);
    close(fd);
}

void print_stats(int fd)
{
    struct tpacket_stats_v3 stats;
    socklen_t len = sizeof(stats);
    int err = getsockopt(fd, SOL_PACKET, PACKET_STATISTICS, &stats, &len);
    if (err < 0)
    {
        perror("getsockopt");
        pthread_exit(NULL);
    }

    fflush(stdout);
    printf("\nReceived %u packets, %lu bytes, %u dropped, freeze_q_cnt: %u\n",
           stats.tp_packets, bytes_total, stats.tp_drops,
           stats.tp_freeze_q_cnt);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-stack-address"
void* poll_socket(void* context)
{
    int fd;
    struct ring ring;
    struct pollfd pfd;
    unsigned int block_num = 0, blocks = 64;
    struct block_desc* pbd;
    struct boyermoore_s matcher;
    
    capture_context_t* ctx = (capture_context_t*) context;
    
    boyermoore_init("8=FIX.4.", &matcher);
    memset(&ring, 0, sizeof(ring));

    ring.fanout_id = ctx->fanout_id;
    ctx->matcher = &matcher;
    fd = setup_socket(&ring, ctx->interface);
    if (fd < 0)
    {
        pthread_exit(NULL);
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;
    int64_t total_blocks = 0;

    while (likely(!sigint))
    {
        pbd = (struct block_desc*) ring.rd[block_num].iov_base;

        if ((pbd->h1.block_status & TP_STATUS_USER) == 0)
        {
            poll(&pfd, 1, -1);
            continue;
        }

        walk_block(ctx, pbd);
        flush_block(pbd);
        block_num = (block_num + 1) % blocks;
        total_blocks++;
    }

    printf("[%d] total blocks: %"PRId64"\n", ctx->thread_num, total_blocks);

    print_stats(fd);

    teardown_socket(&ring, fd);

    pthread_exit(NULL);
}
#pragma clang diagnostic pop
