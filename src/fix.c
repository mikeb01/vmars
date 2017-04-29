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
#include "khash.h"

#define MAX_FRAGMENT_TOTAL_SIZE 4096

typedef struct
{
    const char* s;
    int len;
} str_t;

typedef struct 
{
    int len;
    int capacity;
    char s[0];
} fragment_t;

KHASH_MAP_INIT_INT(str, fragment_t*);

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
    const vmars_capture_context_t* ctx, uint32_t tv_sec, uint32_t tv_nsec, fix_details_t* fix_details)
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

    const size_t summary_header_len = sizeof(vmars_fix_message_summary_t);
    const int key_len = remote_id.len + 1 + local_id.len + 1 + instruction.len + 1 + instrument.len + 1;

    vmars_rb_record_t* record = vmars_spsc_rb_claim(ctx->rb, summary_header_len + key_len);

    if (NULL != record)
    {
        vmars_fix_message_summary_t* summary = (vmars_fix_message_summary_t*) record->data;

        summary->tv_sec = tv_sec;
        summary->tv_nsec = tv_nsec;
        summary->msg_type = (*fix_details).message_type;
        summary->key_len = key_len - 1;

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

        vmars_spsc_rb_publish(ctx->rb, record);
    }
    else
    {
        // TODO: increment counter.
    }
}

static void atomic_release_increment(int64_t* ptr)
{
    __atomic_store_n(ptr, *ptr + 1, __ATOMIC_RELEASE);
}

static fragment_t* get_fragment(vmars_capture_context_t* ctx, int32_t rxhash)
{
    if (NULL == ctx->state)
    {
        return NULL;
    }

    khint_t k;
    khash_t(str)* fragment_map = (khash_t(str)*) ctx->state;

    k = kh_get(str, fragment_map, (khint32_t) rxhash);

    if (k != kh_end(fragment_map))
    {
        return kh_val(fragment_map, k);
    }

    return NULL;
}

static void put_fragment(vmars_capture_context_t* ctx, int32_t rxhash, const char* messsage, int len)
{
    khash_t(str)* fragment_map;
    khint_t k;
    int r;

    if (NULL == ctx->state)
    {
        fragment_map = kh_init(str);
        ctx->state = fragment_map;
    }
    else
    {
        fragment_map = (khash_t(str)*) ctx->state;
    }

    k = kh_get(str, fragment_map, (khint32_t) rxhash);

    fragment_t* fragment;
    if (k != kh_end(fragment_map))
    {
        fragment = kh_val(fragment_map, k);
    }
    else
    {
        fragment = malloc((MAX_FRAGMENT_TOTAL_SIZE * sizeof(char)));
        fragment->capacity = MAX_FRAGMENT_TOTAL_SIZE - sizeof(fragment_t);

        k = kh_put(str, fragment_map, (khint32_t) rxhash, &r);
        kh_val(fragment_map, k) = fragment;
    }

    strncpy(&fragment->s[0], messsage, (size_t) len);
    fragment->len = len;
}

static void remove_fragment(vmars_capture_context_t* ctx, int32_t rxhash)
{
    if (NULL == ctx->state)
    {
        return;
    }

    khint_t k;
    khash_t(str)* fragment_map = (khash_t(str)*) ctx->state;

    k = kh_get(str, fragment_map, (khint32_t) rxhash);

    if (k != kh_end(fragment_map))
    {
        fragment_t* fragment = kh_val(fragment_map, k);
        kh_del(str, fragment_map, k);
        kh_val(fragment_map, k) = NULL;
        free(fragment);
    }
}

int try_parse_fix_message(
    vmars_capture_context_t* ctx,
    const char* fix_messsage,
    int fix_message_len,
    fix_details_t* fix_details)
{
    memset(fix_details, 0, sizeof(fix_details_t));

    int result = parse_fix_message(fix_messsage, fix_message_len, fix_details, NULL, handle_tag, NULL);

    if (result > 0)
    {
        atomic_release_increment(&ctx->counters.valid_messages);
    }
    else if (result == FIX_EMESSAGETOOSHORT)
    {
        // No-op
    }
    else if (result == FIX_ECHECKSUMINVALID)
    {
        atomic_release_increment(&ctx->counters.invalid_checksums);
    }
    else
    {
        atomic_release_increment(&ctx->counters.parsing_errors);
    }

    return result;
}

void vmars_extract_fix_messages(
    vmars_capture_context_t* ctx,
    int32_t rxhash,
    unsigned int tv_sec,
    unsigned int tv_nsec,
    const char* data_ptr,
    size_t data_len)
{
    const char* next_fix_messsage = NULL;
    const char* buf = data_ptr;
    int buf_remaining = (int) data_len;
    fix_details_t fix_details;

    const char* curr_fix_messsage = boyermoore_search(ctx->matcher, buf, buf_remaining);
    // NULL for curr_fix_message is okay here too.

    // data starts with messgage fragment
    if (curr_fix_messsage != buf)
    {
        unsigned long fragment_len = (curr_fix_messsage) ? curr_fix_messsage - buf : data_len;
        buf_remaining -= fragment_len;

        fragment_t* fragment = get_fragment(ctx, rxhash);
        if (NULL == fragment)
        {
            atomic_release_increment(&ctx->counters.corrupt_messages);
        }
        else
        {
            if (fragment->len + fragment_len < fragment->capacity)
            {
                strncpy(&fragment->s[fragment->len], buf, fragment_len);
                fragment->len += fragment_len;

                int result = try_parse_fix_message(ctx, fragment->s, fragment->len, &fix_details);
                if (result > 0)
                {
                    process_for_latency_measurement(ctx, tv_sec, tv_nsec, &fix_details);
                }

                if (result != FIX_EMESSAGETOOSHORT)
                {
                    remove_fragment(ctx, rxhash);
                }
            }
        }
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
            int fix_message_len = NULL == next_fix_messsage ? buf_remaining : (int) (next_fix_messsage - curr_fix_messsage);
            buf_remaining -= fix_message_len;

            int result = try_parse_fix_message(ctx, curr_fix_messsage, fix_message_len, &fix_details);
            if (result > 0)
            {
                process_for_latency_measurement(ctx, tv_sec, tv_nsec, &fix_details);
            }
            else if (FIX_EMESSAGETOOSHORT == result)
            {
                put_fragment(ctx, rxhash, curr_fix_messsage, fix_message_len);
            }
        }

        curr_fix_messsage = next_fix_messsage;
        next_fix_messsage = NULL;
    }
    while (NULL != curr_fix_messsage);
}
