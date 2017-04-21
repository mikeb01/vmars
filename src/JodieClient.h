
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
     int (*init)(char *, int);
     int (*logGauge)(char *, signed long int, signed long int);
     int (*logCounter)(char *, signed long int, signed long int);
     int (*logPercent)(char *, signed long int, signed long int);
     int (*logTiming)(char *, signed long int, signed long int, char *);
     int (*logStatus)(char *, enum status, signed long int);
     int (*flush)( void );
     int (*close)( void );
     struct sockaddr_in jodie_host;
     int jodie_socket;
     unsigned int  send_index;
     unsigned char send_buffer[BUFFER_SIZE];
};

extern struct jodie jodie_server;

/* JodieClient.c */
int jodie_init(char *host, int port);
int jodie_close(void);
signed long int jodie_getMillis(void);
int jodie_logGauge(char *datapath, signed long int value, signed long int timestamp);
int jodie_logCounter(char *datapath, signed long int value, signed long int timestamp);
int jodie_logPercent(char *datapath, signed long int value, signed long int timestamp);
int jodie_logTiming(char *datapath, signed long int value, signed long int timestamp, char *units);
int jodie_logStatus(char *datapath, enum status value, signed long int timestamp);
int jodie_flush(void);
