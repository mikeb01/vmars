#include <x86intrin.h>
#include <string.h>

#include "fixparser.h"

const char* INITIAL_HEADER_42 = "8=FIX.4.2\x01""9=";
const char* INITIAL_HEADER_44 = "8=FIX.4.4\x01""9=";

const int INITIAL_HEADER_LEN = 12;
const int CHECK_SUM_LEN = 7;

// use SSE instructions
static inline int validate_checksum(const char* buf, long buf_len, const char* expected)
{
    register long int sum = 0;
    register long int ii;
    int compare, extra;

    __m64 zero = (__m64) 0LL; // v8qi - 8 bytes
    __m64 dest;
    __m64* pointer;

    // we take 8 bytes at a time through psadbw
    extra = buf_len & 0x07;
    ii = buf_len >> 3;

    pointer = (__m64*) buf;

    while (ii > 0)
    {
        dest = _m_psadbw(*pointer, zero);
        sum += _mm_extract_pi16(dest, 0);
        ii--;
        pointer++;
    }

    // mop up extra characters at the end of the block.
    for (ii = buf_len - extra; ii < buf_len; ii++) sum += buf[ii];

    // convert expected to an int
    compare = ((expected[0] - '0') * 100) + ((expected[1] - '0') * 10) + (expected[2] - '0');

//    if (fix_parser_debug) printf("validate checksum: expected %s, calculated = %ld\n", expected, (sum & 0xff));
    return (sum & 0xff) - compare; // modulo 256 and compare
}


int parse_fix_message(
    const char* buf, int len, void* context,
    void (* startp)(void*, int, int),
    void (* tagp)(void*, int, const char*, int),
    void (* endp)(void*))
{
    int position = 0;
    int fix_type = -1;

    if (INITIAL_HEADER_LEN > len)
    {
        return FIX_EMESSAGETOOSHORT;
    }

    const char* current = buf;

    // what flavour of fix is this?
    if (strncmp(current, INITIAL_HEADER_42, INITIAL_HEADER_LEN) == 0) fix_type = 42;
    if (strncmp(current, INITIAL_HEADER_44, INITIAL_HEADER_LEN) == 0) fix_type = 44;

    if (fix_type == -1) return -1;

    position += INITIAL_HEADER_LEN;

    int msg_len = 0;

    while (position < len)
    {
        char c = buf[position];
        position++;

        if ('0' <= c && c <= '9')
        {
            msg_len = (msg_len * 10) + (c - '0');
        }
        else if (c == '\x01')
        {
            break;
        }
        else
        {
            return FIX_ETAGINVALID;
        }
    }

    if (0 == msg_len || msg_len > (len - position) - CHECK_SUM_LEN)
    {
        return FIX_EMESSAGETOOSHORT;
    }

    if (validate_checksum(&buf[0], msg_len + position, &buf[position + msg_len + 3]) != 0)
    {
        return FIX_ECHECKSUMINVALID;
    }

    if (NULL != startp)
    {
        (startp)(context, msg_len, fix_type);
    }

    do
    {
        int tag = 0;
        while (position < len)
        {
            char b = buf[position];
            position++;
            if ('0' <= b && b <= '9')
            {
                tag = (tag * 10) + (b - '0');
            }
            else if (b == '=')
            {
                break;
            }
            else
            {
                return FIX_ETAGINVALID;
            }
        }

        int valueStartPosition = position;
        int valueEndPosition = -1;

        while (position < len)
        {
            char b = buf[position];
            position++;
            if (b == '\x01')
            {
                valueEndPosition = position - 1;
                break;
            }
        }

        if (valueEndPosition == -1)
        {
            return FIX_EEMPTYVALUE;
        }

        (tagp)(context, tag, &buf[valueStartPosition], valueEndPosition - valueStartPosition);
    }
    while (position < len);

    if (NULL != endp)
    {
        (endp)(context);
    }

    return position;
}

