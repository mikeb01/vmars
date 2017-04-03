#ifndef PACKET_MMAP_SIMPLEBOYERMOORE_H
#define PACKET_MMAP_SIMPLEBOYERMOORE_H

#define TABLE_SIZE 256

struct boyermoore_s
{
    const char* pattern;
    size_t len;
    size_t bad_match[TABLE_SIZE];
};

void boyermoore_init(const char* pattern, struct boyermoore_s* matcher)
{
    matcher->pattern = pattern;
    matcher->len = strlen(pattern);
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        matcher->bad_match[i] = matcher->len;
    }

    for (int i = 0; i < matcher->len - 1; i++)
    {
        matcher->bad_match[pattern[i] & 0xFF] = matcher->len - i - 1;
    }
}

const char* boyermoore_search(struct boyermoore_s* needle, const char* haystack, int length)
{
    if (length < needle->len)
    {
        return NULL;
    }

    int i = 0, j = 0, k = 0;

    for (i = (int) (needle->len - 1); i < length; i += needle->bad_match[haystack[i] & 0xFF])
    {
        for (j = (int) (needle->len - 1), k = i; (j >= 0) && (haystack[k] == needle->pattern[j]); j--)
        {
            k--;
        }
        if (j == -1)
        {
            return &haystack[k + 1];
        }
    }

    return NULL;
}

#endif //PACKET_MMAP_SIMPLEBOYERMOORE_H
