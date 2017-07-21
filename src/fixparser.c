#include <x86intrin.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "fixparser.h"

const char* INITIAL_HEADER_42 = "8=FIX.4.2\x01""9=";
const char* INITIAL_HEADER_44 = "8=FIX.4.4\x01""9=";

const int INITIAL_HEADER_LEN = 12;
const int CHECK_SUM_LEN = 7;

static inline int calculate_checksum(const char* buf, int buf_len)
{
    register long int sum = 0;
    register long int ii;
    int extra;

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

    return (int) (sum & 0xFF);
}

static inline int supplied_checksum(const char* expected)
{
    return ((expected[0] - '0') * 100) + ((expected[1] - '0') * 10) + (expected[2] - '0');
}

// use SSE instructions
static inline int validate_checksum(const char* buf, int buf_len, const char* expected)
{
    const int calulated_checksum = calculate_checksum(buf, buf_len);
    const int compare = supplied_checksum(expected);
    return calulated_checksum - compare; // modulo 256 and compare
}

static bool opt_validate_checksum(int options)
{
    return (options & FIX_OPT_SKIP_VERIFY_CHECKSUM) == 0;
}

static bool opt_report_calculated_checksum(int options)
{
    return (options & FIX_OPT_REPORT_CALCULATED_CHECKSUM_FOR_TAG_10) != 0;
}

vmars_fix_parse_result vmars_fix_parse_msg(
    const char* buf,
    int len,
    void* context,
    void (* startp)(void*, int, int),
    void (* tagp)(void*, int, const char*, int),
    void (* endp)(void*),
    int options)
{
    int position = 0;
    int fix_type = -1;

    if (INITIAL_HEADER_LEN > len)
    {
        return (vmars_fix_parse_result) { .result = FIX_EMESSAGETOOSHORT, .bytes_consumed = len };
    }

    const char* current = buf;

    // what flavour of fix is this?
    if (strncmp(current, INITIAL_HEADER_42, INITIAL_HEADER_LEN) == 0) fix_type = 42;
    if (strncmp(current, INITIAL_HEADER_44, INITIAL_HEADER_LEN) == 0) fix_type = 44;

    if (fix_type == -1) return (vmars_fix_parse_result) { .result = FIX_EINVALIDTYPE, .bytes_consumed = len };

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
            return (vmars_fix_parse_result){ .result = FIX_ETAGINVALID, .bytes_consumed = len };
        }
    }

    if (0 == msg_len || msg_len > (len - position) - CHECK_SUM_LEN)
    {
        return (vmars_fix_parse_result){ .result = FIX_EMESSAGETOOSHORT, .bytes_consumed = len };
    }

    len = msg_len + position + CHECK_SUM_LEN;

    const int calculated_checksum = calculate_checksum(&buf[0], msg_len + position);
    const int checksum = supplied_checksum(&buf[position + msg_len + 3]);

    if (opt_validate_checksum(options) && calculated_checksum != checksum)
//
//    if (validate_checksum(&buf[0], msg_len + position, &buf[position + msg_len + 3]) != 0)
    {
        return (vmars_fix_parse_result){ .result = FIX_ECHECKSUMINVALID, .bytes_consumed = len };
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
                return (vmars_fix_parse_result){ .result = FIX_ETAGINVALID, .bytes_consumed = len };
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
            return (vmars_fix_parse_result){ .result = FIX_EEMPTYVALUE, .bytes_consumed = len };
        }

        if (opt_report_calculated_checksum(options) && tag == 10)
        {
            char calculated_checksum_str[3] = {
                (char) ('0' + (calculated_checksum / 100)),
                (char) ('0' + (calculated_checksum / 10) % 10),
                (char) ('0' + (calculated_checksum % 10))
            };
            (tagp)(context, tag, &calculated_checksum_str[0], 3);
        }
        else
        {
            (tagp)(context, tag, &buf[valueStartPosition], valueEndPosition - valueStartPosition);
        }

    }
    while (position < len);

    if (NULL != endp)
    {
        (endp)(context);
    }

    return (vmars_fix_parse_result){ .result = 0, .bytes_consumed = len };
}

