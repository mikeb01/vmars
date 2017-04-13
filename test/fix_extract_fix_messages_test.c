#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "simpleboyermoore.h"
#include "spsc_rb.h"
#include "packet.h"
#include "fix.h"

static char* fixify(const char* buf)
{
    const size_t buf_len = strlen(buf);
    char* new_buf = (char*) calloc(buf_len + 1, sizeof(char));

    for (int i = 0; i < buf_len; i++)
    {
        if ('|' == buf[i])
        {
            new_buf[i] = '\1';
        }
        else
        {
            new_buf[i] = buf[i];
        }
    }

    return new_buf;
}

static void verify_single_message(capture_context_t* ptr, const char* fix_message, char msg_type, const char* key)
{
    char* buf = fixify(fix_message);
    extract_fix_messages(ptr, buf, strlen(buf));

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
        'D', "BROKER04|REUTERS|ORD10001|HPQ");

    verify_single_message(
        ptr,
        "8=FIX.4.2|9=131|35=D|34=659|49=BROKER04|56=REUTERS|52=20070123-19:09:43|38=1000|59=1|100=N|40=1|11=ORD10001|60=20070123-19:01:17|48=4001|54=1|21=2|10=227|",
        'D', "BROKER04|REUTERS|ORD10001|4001");
}

static void parse_execution_report(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=264|35=8|49=FIX-API|56=user1n3xpr3n09lph|34=4|52=20170412-23:44:20.150|60=20170412-23:44:20.117|20=0|22=8|6=0|11=orderT8B'4ye6sVoCDb|17=QLAKAAAAAAAAAAAC|48=180385|55=instrument1wpxw3fgom2mq|1=1|37=QLAKAAAAAAAAAAAB|151=10.0|14=0.0|38=10.0|54=1|44=100.0|59=0|150=0|39=0|10=232|",
        '8', "user1n3xpr3n09lph|FIX-API|orderT8B'4ye6sVoCDb|instrument1wpxw3fgom2mq");
}

void parse_mass_quote(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=198|35=i|34=4|49=baseUni1ds7nwwsj62ob|52=20170413-01:22:09.595|56=FIX-API|47=P|117=clIdE4c]oav3SF7bg\"L|581=3|296=1|302=1|311=instrument1psh813x11j3i|309=180387|305=8|304=1|295=1|299=0|132=50|134=100000|10=119|",
        'i', "baseUni1ds7nwwsj62ob|FIX-API|clIdE4c]oav3SF7bg\"L|instrument1psh813x11j3i");
}

void parse_mass_quote_ack(capture_context_t* ptr)
{
    verify_single_message(
        ptr,
        "8=FIX.4.2|9=157|35=b|49=FIX-API|56=baseUni1ds7nwwsj62ob|34=4|52=20170413-01:22:09.613|297=0|117=clIdE4c]oav3SF7bg\"L|296=1|302=1|311=instrument1psh813x11j3i|309=180387|305=8|10=019|",
        'b',  "baseUni1ds7nwwsj62ob|FIX-API|clIdE4c]oav3SF7bg\"L|instrument1psh813x11j3i");
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
}
