#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "simpleboyermoore.h"
#include "spsc_rb.h"
#include "counter_handler.h"
#include "packet.h"
#include "fixparser.h"
#include "fix.h"
#include "utils.h"

static void verify_single_message(capture_context_t* ptr, const char* fix_message, msg_type_t msg_type, const char* key)
{
    char* buf = fixify(fix_message);
    extract_fix_messages(ptr, 1, 2, buf, strlen(buf));

    const rb_record_t* msg = spsc_rb_poll(ptr->rb);

    assert(msg != NULL);
    assert(msg->length != 0);

    const fix_message_summary_t* summary = (fix_message_summary_t*) msg->data;

    assert(summary->msg_type == msg_type);
    assert(summary->key_len == strlen(key));
    assert(strncmp(summary->key, key, strlen(key)) == 0);

    spsc_rb_release(ptr->rb, msg);

    free(buf);
}

static void parse_new_order_single(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=130|35=D|34=659|49=BROKER04|56=REUTERS|52=20070123-19:09:43|38=1000|59=1|100=N|40=1|11=ORD10001|60=20070123-19:01:17|55=HPQ|54=1|21=2|10=004|",
        MSG_TYPE_NEW_ORDER_SINGLE, "BROKER04|REUTERS|ORD10001|HPQ");

    verify_single_message(
        ptr,
        "8=FIX.4.2|9=131|35=D|34=659|49=BROKER04|56=REUTERS|52=20070123-19:09:43|38=1000|59=1|100=N|40=1|11=ORD10001|60=20070123-19:01:17|48=4001|54=1|21=2|10=227|",
        MSG_TYPE_NEW_ORDER_SINGLE, "BROKER04|REUTERS|ORD10001|4001");
}

static void parse_execution_report(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=264|35=8|49=FIX-API|56=user1n3xpr3n09lph|34=4|52=20170412-23:44:20.150|60=20170412-23:44:20.117|20=0|22=8|6=0|11=orderT8B'4ye6sVoCDb|17=QLAKAAAAAAAAAAAC|48=180385|55=instrument1wpxw3fgom2mq|1=1|37=QLAKAAAAAAAAAAAB|151=10.0|14=0.0|38=10.0|54=1|44=100.0|59=0|150=0|39=0|10=232|",
        MSG_TYPE_EXECUTION_REPORT, "user1n3xpr3n09lph|FIX-API|orderT8B'4ye6sVoCDb|instrument1wpxw3fgom2mq");
}

void parse_mass_quote(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=198|35=i|34=4|49=baseUni1ds7nwwsj62ob|52=20170413-01:22:09.595|56=FIX-API|47=P|117=clIdE4c]oav3SF7bg\"L|581=3|296=1|302=1|311=instrument1psh813x11j3i|309=180387|305=8|304=1|295=1|299=0|132=50|134=100000|10=119|",
        MSG_TYPE_MASS_QUOTE, "baseUni1ds7nwwsj62ob|FIX-API|clIdE4c]oav3SF7bg\"L|instrument1psh813x11j3i");
}

void parse_mass_quote_ack(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=157|35=b|49=FIX-API|56=baseUni1ds7nwwsj62ob|34=4|52=20170413-01:22:09.613|297=0|117=clIdE4c]oav3SF7bg\"L|296=1|302=1|311=instrument1psh813x11j3i|309=180387|305=8|10=019|",
        MSG_TYPE_MASS_QUOTE_ACK,  "baseUni1ds7nwwsj62ob|FIX-API|clIdE4c]oav3SF7bg\"L|instrument1psh813x11j3i");
}

void parse_trace_request(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=79|35=xr|34=4|49=traceUs18bdsdnueybod|52=20170413-03:25:49.397|56=FIX-API|11=5000|10=201|",
        MSG_TYPE_TRACE_REQ, "traceUs18bdsdnueybod|FIX-API|5000|");
}

void parse_trace_response(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=235|35=xs|49=FIX-API|56=traceUs18bdsdnueybod|34=4|52=20170413-03:25:49.401|11=5000|9100=25704866843180|9101=25704867064367|9102=25704867298900|9103=25704868440546|9104=1492053949399|9105=1492053949400|9106=1492053949400|9107=1492053949401|10=093|",
        MSG_TYPE_TRACE_RSP, "traceUs18bdsdnueybod|FIX-API|5000|");
}

int main()
{
    struct boyermoore_s matcher;
    boyermoore_init("8=FIX.4.", &matcher);
    spsc_rb_t rb;
    spsc_rb_init(&rb, 4096);

    capture_context_t ctx =
    {
        .matcher = &matcher,
        .fanout_id = 0,
        .interface = "eth0",
        .port = 9090,
        .thread_num = 1,
        .rb = &rb
    };

    parse_new_order_single(&ctx);
    parse_execution_report(&ctx);
    parse_mass_quote(&ctx);
    parse_mass_quote_ack(&ctx);
    parse_trace_request(&ctx);
    parse_trace_response(&ctx);
}
