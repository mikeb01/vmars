#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "simpleboyermoore.h"
#include "packet.h"

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

static void parse_simple_message(capture_context_t* ptr)
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

    parse_simple_message(&ctx);
}
