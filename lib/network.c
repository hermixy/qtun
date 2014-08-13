#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "network.h"
#include "qvn.h"

void* network_this;

int create_server(int port, int is_tcp)
{
    server_network_t* network = NULL;
    int yes = 1;
    int i;

    if (conf.type != QVN_CONF_TYPE_SERVER) return QVN_RESULT_WRONG_TYPE;

    network_this = network = malloc(sizeof(server_network_t));
    memset(&network->remote_addr, 0, sizeof(network->remote_addr));
    network->remote_addr.sin_family = AF_INET;
    network->remote_addr.sin_addr.s_addr = INADDR_ANY;
    network->remote_addr.sin_port = htons(port);

    if ((network->bindfd = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        return QVN_STATUS_ERR;
    }

    if (setsockopt(network->bindfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        return QVN_STATUS_ERR;
    }

    if (bind(network->bindfd, (struct sockaddr*)&network->remote_addr, sizeof(network->remote_addr)) == -1)
    {
        perror("bind");
        return QVN_STATUS_ERR;
    }

    if (is_tcp)
    {
        if (listen(network->bindfd, SOMAXCONN) == -1)
        {
            perror("listen");
            return QVN_STATUS_ERR;
        }
        network->connfd = -1;
        network->protocol = NETWORK_PROTOCOL_TCP;
    }
    else
    {
        network->connfd = network->bindfd;
        network->bindfd = -1;
        network->protocol = NETWORK_PROTOCOL_UDP;
    }

    return QVN_STATUS_OK;
}

int create_client(in_addr_t addr, int port, int is_tcp)
{
    client_network_t* network = NULL;
    int yes = 1;
    
    if (conf.type != QVN_CONF_TYPE_CLIENT) return QVN_RESULT_WRONG_TYPE;
    
    network_this = network = malloc(sizeof(client_network_t));
    memset(&network->remote_addr, 0, sizeof(network->remote_addr));
    network->remote_addr.sin_family = AF_INET;
    network->remote_addr.sin_addr.s_addr = addr;
    network->remote_addr.sin_port = htons(port);
    
    if ((network->serverfd = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        return QVN_STATUS_ERR;
    }
    
    if (setsockopt(network->serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        return QVN_STATUS_ERR;
    }
    
    if (connect(network->serverfd, (struct sockaddr*)&network->remote_addr, sizeof(network->remote_addr)) == -1)
    {
        perror("connect");
        return QVN_STATUS_ERR;
    }
    
    return QVN_STATUS_OK;
}

void network_loop()
{
    fd_set set;
    struct timeval tv = {60, 0};
    int max = 0;

    if (conf.type == QVN_CONF_TYPE_SERVER)
    {
        server_network_t* network = network_this;

        while (conf.running)
        {
            FD_ZERO(&set);
            if (network->protocol == NETWORK_PROTOCOL_TCP) FD_SET(network->bindfd, &set);
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
        client_network_t* network = network_this;

        while (conf.running)
        {
            FD_ZERO(&set);
            FD_SET(network->serverfd, &set);
            FD_SET(conf.tun_fd, &set);
            max = network->serverfd > conf.tun_fd ? network->serverfd : conf.tun_fd;
            
            max = select(max + 1, &set, NULL, NULL, &tv);
            if (max != -1) do_network(max, &set);
        }
    }
}

ssize_t write_n(int fd, const void* data, size_t len)
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
            return -1;
        }
        ptr += written;
        left -= written;
    }
    return len;
}

ssize_t read_n(int fd, void* data, size_t len)
{
    char* ptr = data;
    size_t left = len;
    while (left)
    {
        ssize_t readen = read(fd, ptr, left);
        if (readen == 0) return 0;
        else if (readen == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }
        ptr += readen;
        left -= readen;
    }
    return len;
}

inline ssize_t quick_read(int fd, void* data, size_t len)
{
    return read(fd, data, len);
}

inline ssize_t quick_write(int fd, const void* data, size_t len)
{
    return write(fd, data, len);
}
