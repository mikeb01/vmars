#include <string.h>
#include <stdint.h>
#include <packet.h>
#include <assert.h>
#include <stdlib.h>
#include "packet.h"
#include "simpleboyermoore.h"


static char* fixify(char* buf)
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

static void parse_simple_message(capture_context_t* ptr)
{
    char* buf = fixify("8=FIX.4.2|9=130|35=D|34=659|49=BROKER04|56=REUTERS|52=20070123-19:09:43|38=1000|59=1|100=N|40=1|11=ORD10001|60=20070123-19:01:17|55=HPQ|54=1|21=2|10=004|");
    extract_fix_messages(ptr, buf, strlen(buf));

    char* expected_key_string = "BROKER04|REUTERS|ORD1001|HPQ";

    const rb_record_t* msg = spsc_rb_poll(ptr->rb);

    assert(msg != NULL);

    free(buf);
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
