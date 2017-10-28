//
// Created by barkerm on 1/04/17.
//

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <bits/time.h>
#include <time.h>

#define BLOCKSIZE 512
char image[] =
    {
        'P', '5', ' ', '2', '4', ' ', '7', ' ', '1', '5', '\n',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 3, 3, 3, 3, 0, 0, 7, 7, 7, 7, 0, 0,11,11,11,11, 0, 0,15,15,15,15, 0,
        0, 3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0,15, 0, 0,15, 0,
        0, 3, 3, 3, 0, 0, 0, 7, 7, 7, 0, 0, 0,11,11,11, 0, 0, 0,15,15,15,15, 0,
        0, 3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0,15, 0, 0, 0, 0,
        0, 3, 0, 0, 0, 0, 0, 7, 7, 7, 7, 0, 0,11,11,11,11, 0, 0,15, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,
    };

int main()
{
    void *buffer;
    posix_memalign(&buffer, BLOCKSIZE, BLOCKSIZE);
    memcpy(buffer, image, sizeof(image));
    int f = open("feep.pgm", O_CREAT|O_TRUNC|O_WRONLY|O_DIRECT, S_IRWXU);
    struct timespec t0;
    struct timespec t1;

    memset(&t0, 0, sizeof(struct timespec));
    memset(&t1, 0, sizeof(struct timespec));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    int iteration = 1000000;
    for (int i = 0; i < iteration; i++)
    {
        if (-1 == write(f, buffer, BLOCKSIZE))
        {
            printf("Error %s\n", strerror(errno));
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double kPerSec = (double) (iteration / 2) / (double) (t1.tv_sec - t0.tv_sec);

    printf("Time: %ld\n", t1.tv_sec - t0.tv_sec);
    printf("KiB/sec: %f\n", kPerSec);

    close(f);
    free(buffer);
    return 0;
}