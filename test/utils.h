//
// Created by barkerm on 19/04/17.
//

#ifndef PACKET_MMAP_UTILS_H
#define PACKET_MMAP_UTILS_H

char* fixify(const char* buf)
{
    const size_t buf_len = strlen(buf);
    char* new_buf = (char*) calloc(buf_len + 1, sizeof(char));

    for (int i = 0; i < buf_len; i++)
    {
        if ('|' == buf[i])
        {
            new_buf[i] = '\1';
        }
        else
        {
            new_buf[i] = buf[i];
        }
    }

    return new_buf;
}

#endif //PACKET_MMAP_UTILS_H
