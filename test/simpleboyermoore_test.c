#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "simpleboyermoore.h"


bool check_match(struct boyermoore_s* matcher, const char* haystack)
{
    const char* result = boyermoore_search(matcher, haystack, strlen(haystack));
    return result != NULL && strncmp(matcher->pattern, result, matcher->len) == 0;
}


int main(char* argc, char** argv)
{
    struct boyermoore_s matcher;
    boyermoore_init("8=FIX.4.", &matcher);

    // Found
    assert(check_match(&matcher, "8=FIX.4."));
    assert(check_match(&matcher, "....8=FIX.4."));

    // Not found
    assert(!check_match(&matcher, "8=FIX.4"));
    assert(!check_match(&matcher, ""));

    return 0;
}