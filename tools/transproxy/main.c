#include <netdb.h>

#include "network.h"
#include "main.h"

this_t this;

static int init(conf_t conf)
{
    struct hostent* he;
    this.log_level = conf.log_level;
    this.use_udp   = conf.use_udp;
    this.mtu       = conf.mtu;
    this.host      = conf.host;
    this.port      = conf.port;
    this.remote_addr.sin_family = AF_INET;
    this.remote_addr.sin_port   = htons(conf.port);
    if ((he = gethostbyname(conf.host)) == NULL)
    {
        SYSLOG(LOG_ERR, "Convert ip address error");
        return 0;
    }
    this.remote_addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];

    this.buffer    = malloc(conf.mtu);
    if (this.buffer == NULL)
    {
        SYSLOG(LOG_ERR, "not enough memory");
        exit(1);
    }
    this.buffer_len = conf.mtu;

    this.client_fd  = this.remote_fd = -1;

    this.bind_fd    = bind_and_listen(conf.port);
    if (this.bind_fd == -1) return 0;

    return 1;
}

int main(int argc, char* argv[])
{
    conf_t conf;
    conf.log_level = LOG_INFO;
    conf.use_udp   = 1;
    conf.mtu       = 1500;
    conf.host      = "hk2.q-devel.com";
    conf.port      = 6688;

    openlog(argv[0], LOG_PERROR | LOG_CONS | LOG_PID, LOG_LOCAL0);
    if (!init(conf)) return 0;

    while (1)
    {
        loop();
    }
    closelog();
    return 0;
}

