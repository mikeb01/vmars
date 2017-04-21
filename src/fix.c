//
// Created by barkerm on 13/04/17.
//

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "simpleboyermoore.h"
#include "fixparser.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "fix.h"

typedef struct
{
    const char* s;
    int len;
} str_t;

typedef struct
{
    int message_type;
    char ord_status;
    str_t sender_comp_id;
    str_t cl_ord_id;
    str_t target_comp_id;
    str_t symbol;
    str_t security_id;
} fix_details_t;

static void handle_tag(void* context, int fix_tag, const char* fix_value, int len)
{
    fix_details_t* fix_details = (fix_details_t*) context;

    switch (fix_tag)
    {
        case FIX_MSG_TYPE:
            if (len == 1)
            {
                fix_details->message_type = fix_value[0];
            }
            else if (len == 2)
            {
                fix_details->message_type = INT_OF(fix_value[0], fix_value[1]);
            }
            break;

        case FIX_CL_ORD_ID:
            fix_details->cl_ord_id.s = fix_value;
            fix_details->cl_ord_id.len = len;
            break;

        case FIX_QUOTE_ID:
            fix_details->cl_ord_id.s = fix_value;
            fix_details->cl_ord_id.len = len;
            break;

        case FIX_SENDER_COMP_ID:
            fix_details->sender_comp_id.s = fix_value;
            fix_details->sender_comp_id.len = len;
            break;

        case FIX_TARGET_COMP_ID:
            fix_details->target_comp_id.s = fix_value;
            fix_details->target_comp_id.len = len;
            break;

        case FIX_SYMBOL:
            fix_details->symbol.s = fix_value;
            fix_details->symbol.len = len;
            break;

        case FIX_SECURITY_ID:
            fix_details->security_id.s = fix_value;
            fix_details->security_id.len = len;
            break;

        case FIX_UNDERLYING_SECURITY_ID:
            fix_details->security_id.s = fix_value;
            fix_details->security_id.len = len;
            break;

        case FIX_UNDERLYING_SYMBOL:
            fix_details->symbol.s = fix_value;
            fix_details->symbol.len = len;
            break;

        default:
            break;
    }
}

static void process_for_latency_measurement(
    const capture_context_t* ctx, uint32_t tv_sec, uint32_t tv_nsec, fix_details_t* fix_details)
{
    bool should_process = false;
    str_t remote_id = { .s = NULL, .len = 0 };
    str_t local_id = { .s = NULL, .len = 0 };
    str_t instrument = { .s = NULL, .len = 0 };
    str_t instruction = { .s = NULL, .len = 0 };

    switch (fix_details->message_type)
    {
        case MSG_TYPE_NEW_ORDER_SINGLE:
        case MSG_TYPE_MASS_QUOTE:
        case MSG_TYPE_TRACE_REQ:
            should_process = true;
            remote_id = fix_details->sender_comp_id;
            local_id = fix_details->target_comp_id;
            instruction = fix_details->cl_ord_id;

            break;

        case MSG_TYPE_EXECUTION_REPORT:
        case MSG_TYPE_MASS_QUOTE_ACK:
        case MSG_TYPE_TRACE_RSP:
            should_process = true;
            local_id = fix_details->sender_comp_id;
            remote_id = fix_details->target_comp_id;
            instruction = fix_details->cl_ord_id;

            break;

        default:
            break;
    }

    if (!should_process)
    {
        return;
    }

    if (fix_details->symbol.len != 0)
    {
        instrument = fix_details->symbol;
    }
    else if (fix_details->security_id.len != 0)
    {
        instrument = fix_details->security_id;
    }

    const size_t summary_header_len = sizeof(fix_message_summary_t);
    const int key_len = remote_id.len + 1 + local_id.len + 1 + instruction.len + 1 + instrument.len + 1;

    rb_record_t* record = spsc_rb_claim(ctx->rb, summary_header_len + key_len);

    if (NULL != record)
    {
        fix_message_summary_t* summary = (fix_message_summary_t*) record->data;

        summary->tv_sec = tv_sec;
        summary->tv_nsec = tv_nsec;
        summary->msg_type = (*fix_details).message_type;
        summary->key_len = key_len;

        int offset = 0;

        strncpy(&summary->key[offset], remote_id.s, (size_t) remote_id.len);
        offset += remote_id.len;

        summary->key[offset] = '|';
        offset += 1;

        strncpy(&summary->key[offset], local_id.s, (size_t) local_id.len);
        offset += local_id.len;

        summary->key[offset] = '|';
        offset += 1;

        strncpy(&summary->key[offset], instruction.s, (size_t) instruction.len);
        offset += instruction.len;

        summary->key[offset] = '|';
        offset += 1;

        strncpy(&summary->key[offset], instrument.s, (size_t) instrument.len);
        offset += instrument.len;

        summary->key[offset] = '\0';

        spsc_rb_publish(ctx->rb, record);
    }
    else
    {
        // TODO: increment counter.
    }
}

void extract_fix_messages(
    capture_context_t* ctx,
    uint32_t tv_sec, uint32_t tv_nsec,
    const char* data_ptr, size_t data_len)
{
    fix_details_t fix_details;

    const char* next_fix_messsage = NULL;
    const char* buf = data_ptr;
    int buf_remaining = (int) data_len;

    const char* curr_fix_messsage = boyermoore_search(ctx->matcher, buf, buf_remaining);
    // data starts with messgage fragment
    // NULL for curr_fix_message is okay here too.
    if (curr_fix_messsage != buf)
    {
        // Append to existing buffer keyed by rxhash.
    }

    do
    {
        if (NULL != curr_fix_messsage)
        {
            if (buf_remaining > ctx->matcher->len)
            {
                next_fix_messsage = boyermoore_search(
                    ctx->matcher, &curr_fix_messsage[ctx->matcher->len], buf_remaining - (int) ctx->matcher->len);
            }

            // And now for some pointer math.
            int fix_message_len = NULL == next_fix_messsage ? (int) data_len : (int) (next_fix_messsage - curr_fix_messsage);
            buf_remaining -= fix_message_len;

            memset(&fix_details, 0, sizeof(fix_details_t));
            int result = parse_fix_message(curr_fix_messsage, fix_message_len, &fix_details, NULL, handle_tag, NULL);

            if (result > 0)
            {
                process_for_latency_measurement(ctx, tv_sec, tv_nsec, &fix_details);
            }
            else if (result == FIX_EMESSAGETOOSHORT)
            {
                int64_t c = ctx->counters.invalid_checksums;
                __atomic_store_n(&ctx->counters.invalid_checksums, c + 1, __ATOMIC_RELEASE);
                // Copy data to buffer keyed by rxhash.
            }
            else
            {
                // Drop message
            }
        }

        curr_fix_messsage = next_fix_messsage;
        next_fix_messsage = NULL;
    }
    while (NULL != curr_fix_messsage);
}
