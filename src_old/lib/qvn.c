#include <linux/if_tun.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qvn.h"
#include "network.h"
#include "ip_package.h"

#define MAX_PROXY_BLOCK_LENGTH  4096

qvn_conf_t conf;

int init_with_server(int port)
{
    int rc;

    conf.type = QVN_CONF_TYPE_SERVER;
    conf.server.port = port;
    conf.running = 1;

    rc = create_server(port, 0);
    if (rc != QVN_STATUS_OK) return rc;

    memset(conf.tun_name, 0, sizeof(conf.tun_name));
    conf.tun_fd = tun_open(conf.tun_name);
    if (conf.tun_fd == -1) return rc;
    printf("%s opened\n", conf.tun_name);
    
    srand64(microtime64());

    return QVN_STATUS_OK;
}

static int client_say_hello()
{
    client_network_t* network = network_this;
    static char msg[] = "Hello";
    int rc = write_n(network->serverfd, msg, sizeof(msg) - 1);
    if (rc == -1)
    {
        close(network->serverfd);
        perror("write");
        return QVN_STATUS_ERR;
    }
    return QVN_STATUS_OK;
}

int init_with_client(in_addr_t addr, int port)
{
    int rc;
    
    conf.type = QVN_CONF_TYPE_CLIENT;
    conf.client.server = addr;
    conf.client.server_port = port;
    conf.running = 1;
    
    rc = create_client(addr, port, 0);
    if (rc != QVN_STATUS_OK) return rc;
    
    //rc = client_say_hello();
    //if (rc != QVN_STATUS_OK) return rc;
    
    memset(conf.tun_name, 0, sizeof(conf.tun_name));
    conf.tun_fd = tun_open(conf.tun_name);
    if (conf.tun_fd == -1) return rc;
    printf("%s opened\n", conf.tun_name);
    
    srand64(microtime64());
    
    return QVN_STATUS_OK;
}

static void accept_and_check(server_network_t* network)
{
    static char msg[] = "Hello";
    static char dst[sizeof(msg)] = {0};

    int rc;
    int fd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    fd = accept(network->bindfd, (struct sockaddr*)&addr, &len);
    if (fd == -1) return;
    
    memset(dst, 0, sizeof(msg));
    rc = read_n(fd, dst, sizeof(msg) - 1);
    if (rc <= 0) return;
    
    if (strcmp(msg, dst) == 0)
    {
        network->connfd = fd;
        network->remote_addr = addr;
        // TODO: 打印IP地址等信息
    }
    else close(fd);
}

static int do_tun(int tun_fd, int net_fd, int is_tcp, struct sockaddr_in* addr)
{
    static char buf[USHRT_MAX];
    size_t len;
    
    if (read_ip_package(tun_fd, 1, buf, &len) != QVN_STATUS_OK)
    {
        fprintf(stderr, "read ip package faild\n");
        return QVN_STATUS_ERR;
    }
    printf("len: %lu\n", len);
    if (is_tcp) write_n(net_fd, buf, len);
    else sendto(net_fd, buf, len, 0, (struct sockaddr*)addr, sizeof(*addr));
    printf("do_tun\n");
    return QVN_STATUS_OK;
}

static int do_net(int net_fd, int tun_fd)
{
    static char buf[USHRT_MAX];
    size_t len;
    int type;
    struct iovec iv[2];
    
    if (read_ip_package(net_fd, 0, buf, &len) != QVN_STATUS_OK)
    {
        fprintf(stderr, "read ip package faild\n");
        return QVN_STATUS_ERR;
    }
    printf("len: %lu\n", len);
    type = htonl(ETH_P_IP);
    iv[0].iov_base = (char*)&type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = len;
    writev(tun_fd, iv, 2);
    printf("do_net\n");
    return QVN_STATUS_OK;
}

void do_network(int count, fd_set* set)
{
    int rc;
    if (conf.type == QVN_CONF_TYPE_SERVER)
    {
        server_network_t* network = network_this;
        if (FD_ISSET(network->bindfd, set))
        {
            if (network->connfd == -1) accept_and_check(network);
            else
            {
                int fd = accept(network->bindfd, NULL, NULL);
                if (fd != -1)
                {
                    write_n(fd, "error connection", sizeof("error connection") - 1);
                    close(fd);
                }
            }
        }
        if (network->connfd != -1 && FD_ISSET(network->connfd, set)) // 客户端有数据
        {
            rc = do_net(network->connfd, conf.tun_fd);
            if (rc != QVN_STATUS_OK && network->protocol == NETWORK_PROTOCOL_TCP)
            {
                close(network->connfd);
                fprintf(stderr, "connection closed\n");
                network->connfd = -1;
                return;
            }
        }
        if (FD_ISSET(conf.tun_fd, set)) // 服务器有数据
        {
            do_tun(conf.tun_fd, network->connfd, network->protocol == NETWORK_PROTOCOL_TCP, &network->remote_addr);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
        client_network_t* network = network_this;
        if (FD_ISSET(conf.tun_fd, set)) // 客户端有数据
        {
            do_tun(conf.tun_fd, network->serverfd, network->protocol == NETWORK_PROTOCOL_TCP, &network->remote_addr);
        }
        if (FD_ISSET(network->serverfd, set)) // 服务器有数据
        {
            rc = do_net(network->serverfd, conf.tun_fd);
            if (rc != QVN_STATUS_OK)
            {
                close(network->serverfd);
                fprintf(stderr, "connection closed\n");
                conf.running = 0;
                return;
            }
        }
    }
}

