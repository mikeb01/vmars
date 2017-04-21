//
// jodie client in C
// 
// for example usage, see main() at the end.
// 
// gcc -DJODIE_DEBUG -o jodieclient JodieClient.c
// 

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "JodieClient.h"

struct jodie jodie_server =
    {&jodie_init, &jodie_logGauge, &jodie_logCounter, &jodie_logPercent, &jodie_logTiming, &jodie_logStatus,
        &jodie_flush, &jodie_close};


// expects input as an address in numbers and dots format.
// host is required, port is optional. set to 0 to default to JODIE_PORT
// 
int jodie_init(char* host, int port)
{

    // should use getaddrinfo
    if (inet_aton(host, &jodie_server.jodie_host.sin_addr) == 0)
    {
        perror("inet_aton");
        return -1;
    }

    jodie_server.jodie_host.sin_family = AF_INET;

    if (port != 0)
    {
        jodie_server.jodie_host.sin_port = htons(port);
    }
    else
    {
        jodie_server.jodie_host.sin_port = htons(JODIE_PORT);
    }

    jodie_server.jodie_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

    if (jodie_server.jodie_socket == -1)
    {
        perror("socket");
        return -1;
    }

    memset(jodie_server.send_buffer, 0, BUFFER_SIZE);
    jodie_server.send_index = 0;

    return 0;
}

int jodie_close()
{
    return close(jodie_server.jodie_socket);
}

signed long int jodie_getMillis()
{
    struct timeval tt;
    signed long int timestamp;

    gettimeofday(&tt, NULL);

    timestamp = (tt.tv_sec * 1000) + (tt.tv_usec / 1000);

    return timestamp;
}

int jodie_logGauge(char* datapath, signed long int value, signed long int timestamp)
{
    unsigned size_estimate = 0;
    char tmp_buf[BUFFER_SIZE];

    if (strlen(datapath) > (BUFFER_SIZE / 2))
    {
        fprintf(stderr, "%s datapath too long\n", datapath);
        return -1;
    }

    if (timestamp == 0)
    { timestamp = jodie_getMillis(); }

    memset(tmp_buf, 0, BUFFER_SIZE);
    sprintf(tmp_buf, "%s %lu %lu type=gauge\n", datapath, value, timestamp);

    size_estimate = strlen(tmp_buf);

    if ((size_estimate + jodie_server.send_index) > PACKET_SIZE)
    {
        jodie_flush();
        // do a flush
    }

    strncpy((char*) (jodie_server.send_buffer + jodie_server.send_index), tmp_buf, size_estimate);
    jodie_server.send_index += size_estimate;

    return 0;

}

int jodie_logCounter(char* datapath, signed long int value, signed long int timestamp)
{
    unsigned size_estimate = 0;
    char tmp_buf[BUFFER_SIZE];

    if (strlen(datapath) > (BUFFER_SIZE / 2))
    {
        fprintf(stderr, "%s datapath too long\n", datapath);
        return -1;
    }

    if (timestamp == 0)
    { timestamp = jodie_getMillis(); }

    memset(tmp_buf, 0, BUFFER_SIZE);
    sprintf(tmp_buf, "%s %lu %lu type=counter\n", datapath, value, timestamp);

    size_estimate = strlen(tmp_buf);

    if ((size_estimate + jodie_server.send_index) > PACKET_SIZE)
    {
        jodie_flush();
        // do a flush
    }

    strncpy((char*) (jodie_server.send_buffer + jodie_server.send_index), tmp_buf, size_estimate);
    jodie_server.send_index += size_estimate;

    return 0;
}

int jodie_logPercent(char* datapath, signed long int value, signed long int timestamp)
{
    unsigned size_estimate = 0;
    char tmp_buf[BUFFER_SIZE];

    if (strlen(datapath) > (BUFFER_SIZE / 2))
    {
        fprintf(stderr, "%s datapath too long\n", datapath);
        return -1;
    }

    if (timestamp == 0)
    { timestamp = jodie_getMillis(); }

    memset(tmp_buf, 0, BUFFER_SIZE);
    sprintf(tmp_buf, "%s %lu %lu type=percent\n", datapath, value, timestamp);

    if (value > 100)
    { value = 100; }

    size_estimate = strlen(tmp_buf);

    if ((size_estimate + jodie_server.send_index) > PACKET_SIZE)
    {
        jodie_flush();
        // do a flush
    }

    strncpy((char*) (jodie_server.send_buffer + jodie_server.send_index), tmp_buf, size_estimate);
    jodie_server.send_index += size_estimate;

    return 0;
}

int jodie_logTiming(char* datapath, signed long int value, signed long int timestamp, char* units)
{
    unsigned size_estimate = 0;
    char tmp_buf[BUFFER_SIZE];

    if (strlen(datapath) > (BUFFER_SIZE / 2))
    {
        fprintf(stderr, "%s datapath too long\n", datapath);
        return -1;
    }

    if (timestamp == 0)
    { timestamp = jodie_getMillis(); }

    memset(tmp_buf, 0, BUFFER_SIZE);

    if (units == NULL)
    {
        sprintf(tmp_buf, "%s %lu %lu type=timing units=ms\n", datapath, value, timestamp);
    }
    else
    {
        sprintf(tmp_buf, "%s %lu %lu type=timing units=%s\n", datapath, value, timestamp, units);
    }

    size_estimate = strlen(tmp_buf);

    if ((size_estimate + jodie_server.send_index) > PACKET_SIZE)
    {
        jodie_flush();
        // do a flush
    }

    strncpy((char*) (jodie_server.send_buffer + jodie_server.send_index), tmp_buf, size_estimate);
    jodie_server.send_index += size_estimate;

    return 0;
}

// uses the status enum
int jodie_logStatus(char* datapath, enum status value, signed long int timestamp)
{
    unsigned size_estimate = 0;
    char tmp_buf[BUFFER_SIZE];

    if (strlen(datapath) > (BUFFER_SIZE / 2))
    {
        fprintf(stderr, "%s datapath too long\n", datapath);
        return -1;
    }

    if (timestamp == 0)
    { timestamp = jodie_getMillis(); }

    memset(tmp_buf, 0, BUFFER_SIZE);

    sprintf(tmp_buf, "%s %d %lu type=enum\n", datapath, value, timestamp);

    size_estimate = strlen(tmp_buf);

    if ((size_estimate + jodie_server.send_index) > PACKET_SIZE)
    {
        jodie_flush();
        // do a flush
    }

    strncpy((char*) (jodie_server.send_buffer + jodie_server.send_index), tmp_buf, size_estimate);
    jodie_server.send_index += size_estimate;

    return 0;
}


int jodie_flush()
{
    ssize_t retval = 0;

    retval = sendto(
        jodie_server.jodie_socket, jodie_server.send_buffer, strlen((char*) (jodie_server.send_buffer)), 0,
        (const struct sockaddr*) &jodie_server.jodie_host, sizeof(jodie_server.jodie_host));

    if (retval == -1)
    {
        perror("jodie_flush");
        return -1;
    }

    jodie_server.send_index = 0;
    memset(jodie_server.send_buffer, 0, BUFFER_SIZE);

    return 0;
}

#ifdef JODIE_DEBUG
// test main routine
// 
int main (int argc, char *argv[])
{
     enum status test;
     
     test = WARNING;
     
     if (argc < 2) {
      printf ("%s jodie_host\n",argv[0]);
      exit(0);
     }
     
     jodie_server.init(argv[1],0);
     
     jodie_server.logGauge("test.test.foo.gauge",12345,0);
     jodie_server.logCounter("test.test.foo.counter",12345,0);
     jodie_server.logTiming("test.test.foo.timing",12345,0,"ms");
     jodie_server.logStatus("test.test.foo.status",test,0);
     
     jodie_server.flush();
     
     jodie_server.close();
     
     exit(0);
}
#endif