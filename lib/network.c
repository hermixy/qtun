#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
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

    network->connfd = -1;

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
            int max = 0;

            FD_ZERO(&set);
            FD_SET(network->bindfd, &set);
            FD_SET(conf.tun_fd, &set);
            max = network->bindfd > conf.tun_fd ? network->bindfd : conf.tun_fd;

            if (network->connfd != -1)
            {
                FD_SET(network->connfd, &set);
                if (network->connfd > max) max = network->connfd;
            }

            max = select(max + 1, &set, NULL, NULL, &tv);
            if (max != -1) do_network(max, &set);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
    }
}

int s_send(int fd, const void* data, size_t len)
{
    const char* ptr = data;
    size_t left = len;
    while (left)
    {
        ssize_t written = write(fd, ptr, left);
        if (written == 0) return 0;
        else if (written == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return 0;
        }
        ptr += written;
        left -= written;
    }
    return 1;
}

int s_recv(int fd, void* data, size_t len, double timeout)
{
    char* ptr = data;
    size_t left = len;
    double start = microtime();
    while (left)
    {
        ssize_t readen = read(fd, ptr, left);
        if (timeout && microtime() - start >= timeout) return 0;
        if (readen == 0) return 0;
        else if (readen == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return 0;
        }
        ptr += readen;
        left -= readen;
    }
    return 1;
}
