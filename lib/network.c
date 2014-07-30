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
    return QVN_STATUS_OK;
}

void network_loop()
{
    if (conf.type == QVN_CONF_TYPE_SERVER)
    {
        int fd;
        server_network_t* network = network_this;
        struct sockaddr_in addr;
        socklen_t addr_len;
        unsigned char buf[1024] = {0};
        char ip[20] = {0};

        while (conf.running)
        {
            if ((fd = accept(network->bindfd, (struct sockaddr*)&addr, &addr_len)) == -1)
            {
                perror("accept");
                continue;
            }
            getpeername(fd, (struct sockaddr*)&addr, &addr_len);
            inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
            printf("%s accepted\n", ip);
            memset(buf, 0, sizeof(buf));
            while (recv(fd, buf, sizeof(buf), 0) > 0)
            {
                printf("%s\n", buf);
            }
            close(fd);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
    }
}

