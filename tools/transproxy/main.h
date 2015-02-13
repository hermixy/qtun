#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdlib.h>
#include <syslog.h>

typedef struct
{
    int            log_level;
    unsigned char  use_udp;
    unsigned short mtu;
    int            bind_fd;
    int            client_fd; // for tcp
    int            remote_fd;
    unsigned char* buffer;
    size_t         buffer_len;
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

