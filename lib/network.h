#ifndef _NETWORK_H_
#define _NETWORK_H_

typedef struct
{
    int  bindfd;
} server_network_t;

extern void* network_this;

extern int create_server(int port);
extern void network_loop();

#endif

