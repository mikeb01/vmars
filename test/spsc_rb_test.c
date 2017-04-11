//
// Created by barkerm on 9/04/17.
//

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <spsc_rb.h>
#include "spsc_rb.h"

int main(int argc, char** argv)
{
    const char* expected = "Here is a message";
    
    spsc_rb_t rb;
    memset(&rb, 0, sizeof(spsc_rb_t));

    spsc_rb_init(&rb, 1 << 16);

    rb_record_t* record = spsc_rb_claim(&rb, 64);
    
    strncpy((char *) record->data, expected, strlen(expected));

    spsc_rb_publish(&rb, record);

    const rb_record_t* polled = spsc_rb_poll(&rb);

    printf("%s\n", (char*) polled->data);

    assert(polled->length == record->length);
    assert(strncmp(expected, (char *) polled->data, strlen(expected)) == 0);
}