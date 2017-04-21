//
// Created by barkerm on 9/04/17.
//

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <asm/errno.h>

#include "spsc_rb.h"

const char* test_message = "Here is a message";

void publish_and_consume_single_message()
{
    // Given
    vmars_spsc_rb_t rb;
    vmars_spsc_rb_init(&rb, 1 << 16);

    // When
    vmars_rb_record_t* record = vmars_spsc_rb_claim(&rb, 64);
    strncpy((char *) record->data, test_message, strlen(test_message));
    vmars_spsc_rb_publish(&rb, record);
    const vmars_rb_record_t* polled = vmars_spsc_rb_poll(&rb);
    vmars_spsc_rb_release(&rb, polled);

    // Then
    assert(polled != NULL);
    assert(polled->length == record->length);
    assert(strncmp(test_message, (char *) polled->data, strlen(test_message)) == 0);
}


void publish_and_wrap_multiple_messages()
{
    // Given
    vmars_spsc_rb_t rb;
    vmars_spsc_rb_init(&rb, 4096);

    for (int i = 0; i < 1000; i++)
    {
        // When
        vmars_rb_record_t* record = vmars_spsc_rb_claim(&rb, strlen(test_message));
        assert(record != NULL);

        strncpy((char *) record->data, test_message, strlen(test_message));
        vmars_spsc_rb_publish(&rb, record);
        const vmars_rb_record_t* polled = vmars_spsc_rb_poll(&rb);
        assert(vmars_spsc_rb_release(&rb, polled) != EINVAL);

        // Then
        assert(polled != NULL);
        assert(polled->length == record->length);
        assert(strncmp(test_message, (char *) polled->data, strlen(test_message)) == 0);
    }
}

void stop_accepting_messages_when_full()
{
    // Given
    vmars_spsc_rb_t rb;
    vmars_spsc_rb_init(&rb, 4096);

    int num_messages = 4096 / (int) (strlen(test_message) + sizeof(vmars_rb_record_t));

    for (int i = 0; i < num_messages; i++)
    {
        // When
        vmars_rb_record_t* record = vmars_spsc_rb_claim(&rb, strlen(test_message));
        strncpy((char *) record->data, test_message, strlen(test_message));
        vmars_spsc_rb_publish(&rb, record);
    }

    assert(vmars_spsc_rb_claim(&rb, strlen(test_message)) == NULL);
}

void stop_accepting_messages_when_full_when_polled_not_released()
{
    // Given
    vmars_spsc_rb_t rb;
    vmars_spsc_rb_init(&rb, 4096);

    int num_messages = 4096 / (int) (strlen(test_message) + sizeof(vmars_rb_record_t));

    for (int i = 0; i < num_messages; i++)
    {
        // When
        vmars_rb_record_t* record = vmars_spsc_rb_claim(&rb, strlen(test_message));
        strncpy((char *) record->data, test_message, strlen(test_message));
        vmars_spsc_rb_publish(&rb, record);

        vmars_spsc_rb_poll(&rb);
    }

    assert(vmars_spsc_rb_claim(&rb, strlen(test_message)) == NULL);
}

void return_null_when_no_messages_available()
{
    // Given
    vmars_spsc_rb_t rb;
    vmars_spsc_rb_init(&rb, 4096);

    assert(vmars_spsc_rb_poll(&rb) == NULL);
}

int main(int argc, char** argv)
{
    publish_and_consume_single_message();
    publish_and_wrap_multiple_messages();
    stop_accepting_messages_when_full();
    stop_accepting_messages_when_full_when_polled_not_released();
    return_null_when_no_messages_available();
}