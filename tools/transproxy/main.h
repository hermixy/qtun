#ifndef _MAIN_H_
#define _MAIN_H_

#include <arpa/inet.h>
#include <stdlib.h>
#include <syslog.h>

typedef struct
{
    int                log_level;
    unsigned char      use_udp;
    unsigned short     mtu;
    int                bind_fd;
    int                remote_fd;
    unsigned char*     buffer;
    size_t             buffer_len;

    // for tcp
    int                client_fd;
    char*              host;
    unsigned short     port;

    // for udp
    struct sockaddr_in client_addr;
    struct sockaddr_in remote_addr;
} this_t;

extern this_t this;

typedef struct
{
    int            log_level;
    unsigned char  use_udp;
    unsigned short mtu;
    char*          host;
    unsigned short port;
} conf_t;

#define SYSLOG(level, arg...) \
do { \
if (this.log_level >= level) syslog(level, ##arg); \
} while (0)

#endif

