//
// Created by barkerm on 9/04/17.
//

#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "spsc_rb.h"

const char* test_message = "Here is a message";

void publish_and_consume_single_message()
{
    // Given
    spsc_rb_t rb;
    spsc_rb_init(&rb, 1 << 16);

    // When
    rb_record_t* record = spsc_rb_claim(&rb, 64);
    strncpy((char *) record->data, test_message, strlen(test_message));
    spsc_rb_publish(&rb, record);
    const rb_record_t* polled = spsc_rb_poll(&rb);

    // Then
    assert(polled->length == record->length);
    assert(strncmp(test_message, (char *) polled->data, strlen(test_message)) == 0);
}


void publish_and_wrap_multiple_messages()
{
    // Given
    spsc_rb_t rb;
    spsc_rb_init(&rb, 4096);

    for (int i = 0; i < 1000; i++)
    {
        // When
        rb_record_t* record = spsc_rb_claim(&rb, strlen(test_message));
        strncpy((char *) record->data, test_message, strlen(test_message));
        spsc_rb_publish(&rb, record);
        const rb_record_t* polled = spsc_rb_poll(&rb);

        // Then
        assert(polled->length == record->length);
        assert(strncmp(test_message, (char *) polled->data, strlen(test_message)) == 0);
    }
}

int main(int argc, char** argv)
{
    publish_and_consume_single_message();
    publish_and_wrap_multiple_messages();

}