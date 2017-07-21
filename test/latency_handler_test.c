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

void verify_latency_recording_on_mass_quotes_in_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    vmars_fix_message_summary_t* mass_quote = new_summary(1, 0, MSG_TYPE_MASS_QUOTE, "BROKER04|REUTERS|ORD10001|HPQ");
    vmars_fix_message_summary_t* mass_quote_ack = new_summary(1, 500000, MSG_TYPE_MASS_QUOTE_ACK, "BROKER04|REUTERS|ORD10001|HPQ");

    int64_t old_total_count = histogram->total_count;

    calculate_latency(latency_table, histogram, mass_quote);
    calculate_latency(latency_table, histogram, mass_quote_ack);

    assert(histogram->total_count == old_total_count + 1);

    free(mass_quote);
    free(mass_quote_ack);
}

void verify_latency_recording_on_mass_quotes_out_of_order(kh_latency_t* latency_table, struct hdr_histogram* histogram)
{
    vmars_fix_message_summary_t* mass_quote = new_summary(1, 0, MSG_TYPE_MASS_QUOTE, "BROKER04|REUTERS|ORD10001|HPQ");
    vmars_fix_message_summary_t* mass_quote_ack = new_summary(1, 500000, MSG_TYPE_MASS_QUOTE_ACK, "BROKER04|REUTERS|ORD10001|HPQ");

    int64_t old_total_count = histogram->total_count;

    calculate_latency(latency_table, histogram, mass_quote_ack);
    calculate_latency(latency_table, histogram, mass_quote);

    assert(histogram->total_count == old_total_count + 1);

    free(mass_quote);
    free(mass_quote_ack);
}

int main(int argc, char** argv)
{
    khash_t(latency)* latency_table = kh_init(latency);
    struct hdr_histogram* histogram;
    hdr_init(1, INT64_C(10000000000), 2, &histogram);

    verify_latency_recording_on_mass_quotes_in_order(latency_table, histogram);
    verify_latency_recording_on_mass_quotes_out_of_order(latency_table, histogram);

    return 0;
}
