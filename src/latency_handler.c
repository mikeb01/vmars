#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#include "spsc_rb.h"
#include "packet.h"
#include "fix.h"
#include "latency_handler.h"

#define MAX_POLLS 20

static sig_atomic_t sigint = 0;

void latency_sighandler(int num)
{
    sigint = 1;
}

void* poll_ring_buffers(void* context)
{
    buffer_vec_t* buffer_vec = (buffer_vec_t*) context;

    while (!sigint)
    {
        for (int i = 0; i < buffer_vec->len; i++)
        {
            spsc_rb_t* rb = buffer_vec->ring_buffers[i];
            int poll_limit = MAX_POLLS;

            const rb_record_t* record;
            while (NULL != (record = spsc_rb_poll(rb)) && --poll_limit > 0)
            {
                fix_message_summary_t* msg = (fix_message_summary_t*) record->data;
                printf("%d: %.*s\n", msg->msg_type, msg->key_len, msg->key);
                spsc_rb_release(rb, record);
            }
        }
    }

    pthread_exit(NULL);
}
