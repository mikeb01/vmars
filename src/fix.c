//
// Created by barkerm on 13/04/17.
//

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "simpleboyermoore.h"
#include "fixparser.h"
#include "spsc_rb.h"
#include "packet.h"

typedef struct
{
    char message_type;
    char ord_status;
    const char* sender_comp_id;
    int sender_comp_id_len;
    const char* cl_ord_id;
    int cl_ord_id_len;
    const char* target_comp_id;
    int target_comp_id_len;
    const char* symbol;
    int symbol_len;
    const char* security_id;
    int security_id_len;
} fix_details_t;

static void handle_tag(void* context, int fix_tag, const char* fix_value, int len)
{
    fix_details_t* fix_details = (fix_details_t*) context;

    switch (fix_tag)
    {
        case FIX_MSG_TYPE:
            if (len > 0)
            {
                fix_details->message_type = fix_value[0];
            }
            break;

        case FIX_CL_ORD_ID:
            fix_details->cl_ord_id = fix_value;
            fix_details->cl_ord_id_len = len;
            break;

        case FIX_QUOTE_ID:
            fix_details->cl_ord_id = fix_value;
            fix_details->cl_ord_id_len = len;
            break;

        case FIX_SENDER_COMP_ID:
            fix_details->sender_comp_id = fix_value;
            fix_details->sender_comp_id_len = len;
            break;

        case FIX_TARGET_COMP_ID:
            fix_details->target_comp_id = fix_value;
            fix_details->target_comp_id_len = len;
            break;

        case FIX_SYMBOL:
            fix_details->symbol = fix_value;
            fix_details->symbol_len = len;
            break;

        case FIX_SECURITY_ID:
            fix_details->security_id = fix_value;
            fix_details->security_id_len = len;

        default:
            break;
    }
}

static void process_for_latency_measurement(const capture_context_t* ctx, fix_details_t* fix_details)
{
    bool should_process = false;
    const char* remote_id = NULL;
    int remote_id_len = 0;
    const char* local_id = NULL;
    int local_id_len = 0;
    const char* instrument = NULL;
    int instrument_len = 0;
    const char* instruction = NULL;
    int instruction_len = 0;

    switch (fix_details->message_type)
    {
        case 'D':
            should_process = true;

            remote_id = fix_details->sender_comp_id;
            remote_id_len = fix_details->sender_comp_id_len;
            local_id = fix_details->target_comp_id;
            local_id_len = fix_details->target_comp_id_len;
            instruction = fix_details->cl_ord_id;
            instruction_len = fix_details->cl_ord_id_len;

            break;

        case '8':
            should_process = true;

            local_id = fix_details->sender_comp_id;
            local_id_len = fix_details->sender_comp_id_len;
            remote_id = fix_details->target_comp_id;
            remote_id_len = fix_details->target_comp_id_len;
            instruction = fix_details->cl_ord_id;
            instruction_len = fix_details->cl_ord_id_len;

            break;

        default:
            break;
    }

    if (!should_process)
    {
        return;
    }

    if (fix_details->symbol_len != 0)
    {
        instrument = fix_details->symbol;
        instrument_len = fix_details->symbol_len;
    }
    else if (fix_details->security_id_len != 0)
    {
        instrument = fix_details->security_id;
        instrument_len = fix_details->security_id_len;
    }

    const size_t summary_header_len = sizeof(fix_message_summary_t);
    const int key_len = remote_id_len + 1 + local_id_len + 1 + instruction_len + 1 + instrument_len;

    rb_record_t* record = spsc_rb_claim(ctx->rb, summary_header_len + key_len);

    if (NULL != record)
    {
        fix_message_summary_t* summary = (fix_message_summary_t*) record->data;

        summary->tv_sec = 1;
        summary->tv_nsec = 2;
        summary->msg_type = (*fix_details).message_type;
        summary->key_len = key_len;

        int offset = 0;
        int len = remote_id_len;

        strncpy(&summary->key[offset], remote_id, (size_t) len);
        offset += len;

        summary->key[offset] = '|';
        offset += 1;

        len = local_id_len;
        strncpy(&summary->key[offset], local_id, (size_t) len);
        offset += len;

        summary->key[offset] = '|';
        offset += 1;

        len = instruction_len;
        strncpy(&summary->key[offset], instruction, (size_t) len);
        offset += len;

        summary->key[offset] = '|';
        offset += 1;

        len = instrument_len;
        strncpy(&summary->key[offset], instrument, (size_t) len);

        spsc_rb_publish(ctx->rb, record);
    }
    else
    {
        // TODO: increment counter.
    }
}

void extract_fix_messages(const capture_context_t* ctx, const char* data_ptr, size_t data_len)
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
                process_for_latency_measurement(ctx, &fix_details);
            }
            else if (result == FIX_EMESSAGETOOSHORT)
            {
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
