//
// Created by barkerm on 1/04/17.
//

// Taken from the example at:
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
#include <errno.h>

#include "common.h"
#include "simpleboyermoore.h"
#include "spsc_rb.h"
#include "counter_handler.h"
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
void vmars_packet_sighandler(int num)
{
    sigint = 1;
}
#pragma clang diagnostic pop

static int setup_socket(struct ring* ring, const char* netdev, int use_hw_timestamps)
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
        perror("Failed open SOCK_RAW socket");
        return -1;
    }

    err = setsockopt(fd, SOL_PACKET, PACKET_VERSION, &v, sizeof(v));
    if (err < 0)
    {
        perror("Failed to setsockopt to SOL_PACKET.PACKET_VERSION=TPACKET_V3");
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
        perror("Failed to setsockopt to SOL_PACKET.PACKET_RX_RING=...");
        return -1;
    }

    ring->map = mmap(
        NULL, ring->req.tp_block_size * ring->req.tp_block_nr, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
    if (ring->map == MAP_FAILED)
    {
        perror("Failed to memory map PF_RING");
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
        fprintf(stderr, "Failed to set fanout option for socket, interface: %s, error: %s\n", netdev, strerror(errno));
        return -1;
    }

    memset(&hwtstamp, 0, sizeof(struct ifreq));
    memset(&hwconfig, 0, sizeof(struct hwtstamp_config));

    strncpy(hwtstamp.ifr_name, netdev, sizeof(hwtstamp.ifr_name));
    hwtstamp.ifr_data = (void *)&hwconfig;
    hwconfig.rx_filter = HWTSTAMP_FILTER_ALL;

    if (use_hw_timestamps)
    {
        if (ioctl (fd, SIOCSHWTSTAMP, &hwtstamp) < 0)
        {
            perror("SIOCGHWTSTAMP failed - not using HW timestamps");
        }
        else
        {
            int req = SOF_TIMESTAMPING_RAW_HARDWARE;
            if (setsockopt(fd, SOL_PACKET, PACKET_TIMESTAMP, (void *) &req, sizeof(req)) < 0)
            {
                perror("Unable to setsockopt for hardware timestamping");
            }
            else
            {
                printf("Hardware timestamps enabled on: %s\n", netdev);
            }
        }
    }

    return fd;
}

static void display(vmars_capture_context_t* ctx, struct tpacket3_hdr* ppd)
{
    struct ethhdr* eth = (struct ethhdr*) ((uint8_t*) ppd + ppd->tp_mac);
    struct iphdr* ip = (struct iphdr*) ((uint8_t*) eth + ETH_HLEN);

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

    vmars_extract_fix_messages(
        ctx, ip->saddr, tcp->source, ip->daddr, tcp->dest, ppd->tp_sec, ppd->tp_nsec, data_ptr, data_len);
}

static void walk_block(vmars_capture_context_t* ctx, struct block_desc* pbd)
{
    int num_pkts = pbd->h1.num_pkts, i;
    unsigned long bytes = 0;
    struct tpacket3_hdr* ppd;

    ppd = (struct tpacket3_hdr*) ((uint8_t*) pbd + pbd->h1.offset_to_first_pkt);
    for (i = 0; i < num_pkts; ++i)
    {
        bytes += ppd->tp_snaplen;
        display(ctx, ppd);

        ppd = (struct tpacket3_hdr*) ((uint8_t*) ppd + ppd->tp_next_offset);
    }

    int64_t c = ctx->counters.bytes_total;
    __atomic_store_n(&ctx->counters.bytes_total, c + 1, __ATOMIC_RELEASE);
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-stack-address"
void* vmars_poll_socket(void* context)
{
    int fd;
    struct ring ring;
    struct pollfd pfd;
    unsigned int block_num = 0, blocks = 64;
    struct block_desc* pbd;
    struct boyermoore_s matcher;
    
    vmars_capture_context_t* ctx = (vmars_capture_context_t*) context;

    ctx->cpu_num;

    vmars_verbose(
        "[%d] Starting capture thread on interface: %s, fanout id: %d, cpu: %d\n",
        ctx->thread_num, ctx->interface, ctx->fanout_id, ctx->cpu_num);

    if (-1 < ctx->cpu_num)
    {
        pthread_t self = pthread_self();
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(ctx->cpu_num, &cpu_set);
        int result = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpu_set);
        if (result < 0)
        {
            fprintf(
                stderr,
                "[%d] Failed to set thread affinity to cpu: %d, %s[%d].  Continuing anyway...",
                ctx->thread_num, ctx->cpu_num, strerror(result), result);
        }
        else
        {
            vmars_verbose("[%d] Set affinity for thread to CPU: %d\n", ctx->thread_num, ctx->cpu_num);
        }
    }
    
    boyermoore_init("8=FIX.4.", &matcher);
    memset(&ring, 0, sizeof(ring));

    ring.fanout_id = ctx->fanout_id;
    ctx->matcher = &matcher;
    fd = setup_socket(&ring, ctx->interface, ctx->config->use_hw_timestamps);
    if (fd < 0)
    {
        pthread_exit(NULL);
    }

    __atomic_store_n(&ctx->counters.fd, fd, __ATOMIC_SEQ_CST);

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;

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
    }

    teardown_socket(&ring, fd);

    pthread_exit(NULL);
}
#pragma clang diagnostic pop
