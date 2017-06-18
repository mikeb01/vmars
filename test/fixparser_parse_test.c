//
// Created by barkerm on 19/04/17.
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fixparser.h>
#include <stdio.h>

#include "fixparser.h"
#include "utils.h"

void start(void* a, int b, int c) {}

void tag(void* a, int b, const char* c, int d) {}

void print_checksum_callback(void* a, int tag, const char* val, int len)
{
    if (tag == 10)
    {
        printf("%.3s", val);
    }
}


void end(void* a) {}

#define TEST_EXECUTION_REPORT_TRADE "8=FIX.4.2|9=264|35=8|49=FIX-API|56=user1n3xpr3n09lph|34=4|52=20170412-23:44:20.150|60=20170412-23:44:20.117|20=0|22=8|6=0|11=orderT8B'4ye6sVoCDb|17=QLAKAAAAAAAAAAAC|48=180385|55=instrument1wpxw3fgom2mq|1=1|37=QLAKAAAAAAAAAAAB|151=10.0|14=0.0|38=10.0|54=1|44=100.0|59=0|150=F|39=0|10=232|"

void print_tag(const char* c)
{
    vmars_fix_parse_msg(
        c, strlen(c), NULL, start, print_checksum_callback, end,
        FIX_OPT_SKIP_VERIFY_CHECKSUM | FIX_OPT_REPORT_CALCULATED_CHECKSUM_FOR_TAG_10);
}

int main(int argc, char** argv)
{
    const char* fix_message =
        "8=FIX.4.2|9=224|35=8|49=REUTERS|56=BROKER04|34=4|52=20170412-23:44:20.150|60=20170412-23:44:20.117|20=0|"
        "22=8|6=0|11=ORD10001|17=QLAKAAAAAAAAAAAC|48=180385|55=HPQ|1=1|37=QLAKAAAAAAAAAAAB|151=10.0|14=0.0|38=10.0|"
        "54=1|44=100.0|59=0|150=0|39=0|10=010|8=FIX.4.2|9=224|35=8";

    char* buf = fixify(fix_message);

    vmars_fix_parse_result result = vmars_fix_parse_msg(buf, strlen(buf), NULL, start, tag, end, 0);
    assert(result.result == 0);
    assert(result.bytes_consumed == 224 + 16 + 7);

    print_tag(fixify(TEST_EXECUTION_REPORT_TRADE));
}