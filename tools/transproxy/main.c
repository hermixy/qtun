#include "network.h"
#include "main.h"

this_t this;

static int init(conf_t conf)
{
    this.log_level = conf.log_level;
    this.use_udp   = conf.use_udp;
    this.mtu       = conf.mtu;

    this.buffer    = malloc(conf.mtu);
    if (this.buffer == NULL)
    {
        SYSLOG(LOG_ERR, "not enough memory");
        exit(1);
    }
    this.buffer_len = conf.mtu;

    this.client_fd  = -1;

    this.bind_fd    = bind_and_listen(conf.port);
    if (this.bind_fd == -1) return 0;

    this.remote_fd  = connect_server(conf.host, conf.port);
    if (this.remote_fd == -1) return 0;

    return 1;
}

int main(int argc, char* argv[])
{
    conf_t conf;
    conf.log_level = LOG_WARNING;
    conf.use_udp   = 0;
    conf.mtu       = 1500;
    conf.host      = "hk2.q-devel.com";
    conf.port      = 6687;

    if (!init(conf)) return 0;

    loop();
    return 0;
}

