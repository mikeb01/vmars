#ifndef PACKET_MMAP_SIMPLEBOYERMOORE_H
#define PACKET_MMAP_SIMPLEBOYERMOORE_H

#define TABLE_SIZE 256

struct boyermoore_s
{
    const char* pattern;
    size_t len;
    size_t bad_match[TABLE_SIZE];
};

void boyermoore_init(const char* pattern, struct boyermoore_s* matcher);

const char* boyermoore_search(struct boyermoore_s* needle, const char* haystack, int length);

#endif //PACKET_MMAP_SIMPLEBOYERMOORE_H
