#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "qvn.h"

void* network_this;

int create_server(int port)
{
    server_network_t* network = NULL;
    struct sockaddr_in addr = {0};
    int yes = 1;
    int i;

    if (conf.type != QVN_CONF_TYPE_SERVER) return QVN_RESULT_WRONG_TYPE;

    network_this = network = malloc(sizeof(server_network_t));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if ((network->bindfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return QVN_STATUS_ERR;
    }

    if (setsockopt(network->bindfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        return QVN_STATUS_ERR;
    }

    if (bind(network->bindfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        return QVN_STATUS_ERR;
    }

    if (listen(network->bindfd, SOMAXCONN) == -1)
    {
        perror("listen");
        return QVN_STATUS_ERR;
    }

    for (i = 0; i < MAX_CLIENTS; ++i) network->clients[i] = -1;

    return QVN_STATUS_OK;
}

void network_loop()
{
    fd_set set;
    struct timeval tv = {60, 0};
    int i;

    if (conf.type == QVN_CONF_TYPE_SERVER)
    {
        server_network_t* network = network_this;

        while (conf.running)
        {
            int max;

            FD_ZERO(&set);
            FD_SET(network->bindfd, &set);
            FD_SET(conf.tun_fd, &set);
            max = network->bindfd > conf.tun_fd ? network->bindfd : conf.tun_fd;

            for (i = 0; i < MAX_CLIENTS; ++i)
            {
                if (network->clients[i] != -1)
                {
                    FD_SET(network->clients[i], &set);
                    if (network->clients[i] > max) max = network->clients[i];
                }
            }

            max = select(max + 1, &set, NULL, NULL, &tv);
            if (max == -1)
            else do_network(max, &set);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
    }
}

