#include <linux/if_tun.h>
#include <stdio.h>
#include <string.h>

#include "qvn.h"
#include "network.h"

qvn_conf_t conf;

int init_with_server(int port)
{
    int rc;

    conf.type = QVN_CONF_TYPE_SERVER;
    conf.server.port = port;
    conf.running = 1;

    rc = create_server(port);
    if (rc != QVN_STATUS_OK) return rc;

    memset(conf.tun_name, 0, sizeof(conf.tun_name));
    conf.tun_fd = tun_open(conf.tun_name);
    if (conf.tun_fd == -1) return rc;
    printf("%s opened\n", conf.tun_name);

    return QVN_STATUS_OK;
}

void do_network(int count, fd_set* set)
{
    if (conf.type == QVN_CONF_TYPE_SERVER)
    {
        server_network_t* network = network_this;
        if (FD_ISSET(network->bindfd, set))
        {
            network->clients[0] = accept(network->bindfd, NULL, NULL);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
    }
}

