
#define BUFFER_SIZE 2048
#define PACKET_SIZE 1400
#define JODIE_PORT 41000


enum status { 
     OK = 0,
     WARNING = 1,
     CRITICAL = 2,
     UNKNOWN = 3,
     NORESPONSE = 4, 
     EXCHANGECLOSED = 5 
};

struct jodie {
     struct sockaddr_in jodie_host;
     int jodie_socket;
     unsigned int  send_index;
     unsigned char send_buffer[BUFFER_SIZE];
};

extern struct jodie jodie_server;

/* JodieClient.c */
int jodie_init(char *host, int port, struct jodie* jodie_server);
int jodie_dup(struct jodie* src, struct jodie* dst);
int jodie_close(struct jodie* jodie_server);
signed long int jodie_getMillis(void);
int jodie_logGauge(struct jodie* jodie_server, char *datapath, signed long int value, signed long int timestamp);
int jodie_logCounter(struct jodie* jodie_server, char *datapath, signed long int value, signed long int timestamp);
int jodie_logPercent(struct jodie* jodie_server, char *datapath, signed long int value, signed long int timestamp);
int jodie_logTiming(struct jodie* jodie_server, char *datapath, signed long int value, signed long int timestamp, char *units);
int jodie_logStatus(struct jodie* jodie_server, char *datapath, enum status value, signed long int timestamp);
int jodie_flush(struct jodie* jodie_server);
