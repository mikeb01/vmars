//
// Created by barkerm on 21/07/17.
//

#include <stdint.h>
#include <assert.h>
#include <hdr_histogram.h>
#include <khash.h>

#include "common.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "fix.h"
#include "fixparser.h"

#include "latency_handler.h"

KHASH_MAP_INIT_STR(latency, latency_record_t*);

void calculate_latency(kh_latency_t* latency_table, struct hdr_histogram* histogram, vmars_fix_message_summary_t* msg);


vmars_fix_message_summary_t* new_summary(int tv_sec, int tv_nsec, vmars_fix_msg_type_t type, const char* key)
{
    size_t len = strlen(key);

    vmars_fix_message_summary_t* summary = malloc(sizeof(vmars_fix_message_summary_t) + len + 1);

    summary->tv_sec = tv_sec;
    summary->tv_nsec = tv_nsec;
    summary->msg_type = type;
    summary->key_len = (int32_t) len;

    strncpy(summary->key, key, len);

    return summary;
}

int64_t count_latency_recording_for_message_types(
    kh_latency_t* latency_table, struct hdr_histogram* histogram,
    vmars_fix_msg_type_t msg_type_a, int tv_nsec_a,
    vmars_fix_msg_type_t msg_type_b, int tv_nsec_b)
{
    vmars_fix_message_summary_t* msg_a = new_summary(1, tv_nsec_a, msg_type_a, "BROKER04|REUTERS|ORD10001|HPQ");
    vmars_fix_message_summary_t* msg_b = new_summary(1, tv_nsec_b, msg_type_b, "BROKER04|REUTERS|ORD10001|HPQ");

    int64_t old_total_count = histogram->total_count;

    calculate_latency(latency_table, histogram, msg_a);
    calculate_latency(latency_table, histogram, msg_b);

    free(msg_a);
    free(msg_b);

    return histogram->total_count - old_total_count;
}

void verify_latency_recording_on_mass_quotes_in_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_MASS_QUOTE, 0, MSG_TYPE_MASS_QUOTE_ACK, 500000));
}

void verify_latency_recording_on_mass_quotes_out_of_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_MASS_QUOTE_ACK, 500000, MSG_TYPE_MASS_QUOTE, 0));
}

void verify_latency_recording_on_single_order_in_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_NEW_ORDER_SINGLE, 0, MSG_TYPE_EXECUTION_REPORT, 500000));
}

void verify_latency_recording_on_single_order_out_of_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_EXECUTION_REPORT, 500000, MSG_TYPE_NEW_ORDER_SINGLE, 0));
}

void verify_latency_recording_on_trace_in_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_TRACE_REQ, 0, MSG_TYPE_TRACE_RSP, 500000));
}

void verify_latency_recording_on_trace_out_of_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    assert(1 == count_latency_recording_for_message_types(
        latency_table, histogram, MSG_TYPE_TRACE_RSP, 500000, MSG_TYPE_TRACE_REQ, 0));
}


int main(int argc, char** argv)
{
    khash_t(latency)* latency_table = kh_init(latency);
    struct hdr_histogram* histogram;
    hdr_init(1, INT64_C(10000000000), 2, &histogram);

    verify_latency_recording_on_mass_quotes_in_order(latency_table, histogram);
    verify_latency_recording_on_mass_quotes_out_of_order(latency_table, histogram);
    verify_latency_recording_on_single_order_in_order(latency_table, histogram);
    verify_latency_recording_on_single_order_out_of_order(latency_table, histogram);
    verify_latency_recording_on_trace_in_order(latency_table, histogram);
    verify_latency_recording_on_trace_out_of_order(latency_table, histogram);

    return 0;
}
