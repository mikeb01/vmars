//
// Created by barkerm on 9/04/17.
//

#define _GNU_SOURCE

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "spsc_rb.h"

static bool is_power_of_two(size_t value)
{
    return (value > 0 && ((value & (~value + 1)) == value));
}

static bool is_multiple_of_page_size(size_t value)
{
    return ((getpagesize() - 1) & value) == 0;
}

static void* map_buffer(size_t length)
{

    // First we need some contiguous virtual memory that is twice the size of the desired memory segment that
    // we want to size.

    // This could be done with malloc, while that seems to work (on Linux), some operating
    // systems may separate it memory regions so that an address returned from malloc may not
    // support memory-mapped files.  So I am going to use an anonymous mmap to do the initial allocation.

    void* address = NULL;

    void *full_address = mmap(0, length * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == full_address)
    {
        fprintf(stderr, "Failed to create anon memory map: %s[%d]\n", strerror(errno), errno);
        return NULL;
    }

    // Next we map the real file twice into same block of contigious virtual memory that we
    // previously allocated.

    char name[] = "/dev/shm/spsc_rb_XXXXXX";
    int fd = mkstemp(name);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to create shared memory: %s[%d]\n", strerror(errno), errno);
        return NULL;
    }

    // Make sure the file is the same size as the buffer, otherwise you will get bus errors.
    ftruncate(fd, length);

    // Overwrite the existing mapping with our new one.  To make this work we need to use the MAP_FIXED, which
    // the use of which is generally discouraged.  Not sure if it even supported on Windows.

    void* first_half_address = mmap(full_address, length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    if (MAP_FAILED == first_half_address)
    {
        fprintf(stderr, "Failed to map real file: %s[%d]\n", strerror(errno), errno);
        goto cleanup;
    }

    // Map the second half of the virtual memory region to the same file.

    void* second_half_address = mmap(full_address + length, length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    if (MAP_FAILED == second_half_address)
    {
        fprintf(stderr, "Failed to map real file: %s[%d]\n", strerror(errno), errno);
        goto cleanup;
    }

    address = first_half_address;

    cleanup:
        close(fd);
        unlink(name);

    return address;
}

int vmars_spsc_rb_init(vmars_spsc_rb_t* rb, size_t capacity)
{
    if (!is_power_of_two(capacity) && is_multiple_of_page_size(capacity))
    {
        return EINVAL;
    }

    rb->buffer = map_buffer(capacity);
    rb->capacity = capacity;
    rb->descriptor = calloc(1, sizeof(vmars_rb_descriptor_t));

    if (rb->buffer == NULL || rb->descriptor == NULL)
    {
        return ENOMEM;
    }

    return 0;
}

vmars_rb_record_t* vmars_spsc_rb_claim(vmars_spsc_rb_t* rb, size_t length)
{
    const size_t record_lenth = length + sizeof(vmars_rb_record_t);
    const size_t mask = rb->capacity - 1;
    const size_t required_capacity = record_lenth; // Cache align maybe.

    int64_t head = rb->descriptor->head_cache_position;
    int64_t tail = rb->descriptor->tail_position;
    const int32_t available_capacity = (int32_t)rb->capacity - (int32_t)(tail - head);

    size_t padding = 0;
    size_t tail_index = (size_t)tail & mask;
    const size_t to_buffer_end_length = rb->capacity - tail_index;
    vmars_rb_record_t *record = NULL;

    if (length > rb->capacity)
    {
        errno = EINVAL;
        return NULL;
    }

    if ((int32_t) required_capacity > available_capacity)
    {
        head = __atomic_load_n(&rb->descriptor->head_position, __ATOMIC_SEQ_CST);

        if (required_capacity > (rb->capacity - (size_t)(tail - head)))
        {
            return NULL;
        }

        rb->descriptor->head_cache_position = head;
    }

    record = (vmars_rb_record_t *)(rb->buffer + tail_index);
    record->length = (int32_t) length;

    return record;
}

int vmars_spsc_rb_publish(vmars_spsc_rb_t* rb, vmars_rb_record_t* to_publish)
{
    const size_t mask = rb->capacity - 1;
    const int64_t tail = rb->descriptor->tail_position;
    const size_t tail_index = (size_t)tail & mask;

    if (to_publish != (vmars_rb_record_t *)(rb->buffer + tail_index))
    {
        return EINVAL;
    }

    int64_t new_tail = tail + to_publish->length + sizeof(vmars_rb_record_t);
    __atomic_store_n(&rb->descriptor->tail_position, new_tail, __ATOMIC_SEQ_CST);

    return 0;
}

const vmars_rb_record_t* vmars_spsc_rb_poll(vmars_spsc_rb_t* rb)
{
    const int64_t head = rb->descriptor->head_position;
    const size_t head_index = (int32_t)head & (rb->capacity - 1);
    
    int64_t tail = rb->descriptor->tail_cache_position;

    if (0 >= tail - head)
    {
        tail = __atomic_load_n(&rb->descriptor->tail_position, __ATOMIC_SEQ_CST);    
        
        if (0 >= tail - head)
        {
            return NULL;
        }
        
        rb->descriptor->tail_cache_position = tail;
    }

    const vmars_rb_record_t* record = (vmars_rb_record_t*) (rb->buffer + head_index);

    return record;
}

int vmars_spsc_rb_release(vmars_spsc_rb_t* rb, const vmars_rb_record_t* to_release)
{
    const int64_t head = rb->descriptor->head_position;
    const size_t head_index = (int32_t)head & (rb->capacity - 1);
    
    if (to_release != (vmars_rb_record_t*) (rb->buffer + head_index))
    {
        return EINVAL;
    }

    const size_t new_head = head + sizeof(vmars_rb_record_t) + to_release->length;

    __atomic_store_n(&rb->descriptor->head_position, new_head, __ATOMIC_SEQ_CST);

    return 0;
}
