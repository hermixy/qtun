#ifndef _NETWORK_H_
#define _NETWORK_H_

extern int bind_and_listen(unsigned short port);
extern int connect_server(char* host, unsigned short port);
extern void loop();

#endif

