//
// Created by barkerm on 31/07/17.
//


#include <stdarg.h>
#include <stdio.h>

void vmars_verbose(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
