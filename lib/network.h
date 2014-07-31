#ifndef _NETWORK_H_
#define _NETWORK_H_

#define MAX_CLIENTS   100

typedef struct
{
    int  bindfd;
    int  clients[MAX_CLIENTS];
} server_network_t;

extern void* network_this;

extern int create_server(int port);
extern void network_loop();

#endif

