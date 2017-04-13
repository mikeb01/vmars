#ifndef _FIXPARSER_H
#define _FIXPARSER_H

#define FIX_CL_ORD_ID 11
#define FIX_MSG_TYPE 35
#define FIX_SECURITY_ID 48
#define FIX_SENDER_COMP_ID 49
#define FIX_SYMBOL 55
#define FIX_TARGET_COMP_ID 56
#define FIX_QUOTE_ID 117
#define FIX_UNDERLYING_SECURITY_ID 309
#define FIX_UNDERLYING_SYMBOL 311

#define FIX_EMESSAGETOOSHORT -1
#define FIX_ECHECKSUMINVALID -2
#define FIX_ETAGINVALID -3
#define FIX_EEMPTYVALUE -4

#define INT_OF(x, y) (((int) x << 8) + y)

typedef enum
{
    MSG_TYPE_NEW_ORDER_SINGLE = (int) 'D',
    MSG_TYPE_EXECUTION_REPORT = (int) '8',
    MSG_TYPE_MASS_QUOTE = (int) 'i',
    MSG_TYPE_MASS_QUOTE_ACK = (int) 'b',
    MSG_TYPE_TRACE_REQ = INT_OF('x', 'r'),
    MSG_TYPE_TRACE_RSP = INT_OF('x', 's')
} msg_type_t;

// prototypes
int parse_fix_message(
    const char* buf, int len,
    void* context,
    void (* startp)(void*, int, int),
    void (* tagp)(void*, int, const char*, int),
    void (* endp)(void*));

/* fix.c */

#endif // FIXPARSER_H
