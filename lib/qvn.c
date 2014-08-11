#include <linux/if_tun.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qvn.h"
#include "network.h"

#define MAX_PROXY_BLOCK_LENGTH  4096

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
    
    rc = create_client(addr, port);
    if (rc != QVN_STATUS_OK) return rc;
    
    rc = client_say_hello();
    if (rc != QVN_STATUS_OK) return rc;
    
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
    int fd = accept(network->bindfd, NULL, NULL);
    if (fd == -1) return;
    
    memset(dst, 0, sizeof(msg));
    rc = read_n(fd, dst, sizeof(msg) - 1);
    if (rc <= 0) return;
    
    if (strcmp(msg, dst) == 0)
    {
        network->connfd = fd;
        // TODO: 打印IP地址等信息
    }
    else close(fd);
}

static void do_tun(int tun_fd, int net_fd)
{
    char* buf;
    ssize_t ret;
    unsigned short want_len;
    
    want_len = rand32() % MAX_PROXY_BLOCK_LENGTH;
    buf = malloc(want_len + sizeof(want_len));
    ret = quick_read(tun_fd, buf + sizeof(want_len), want_len);
    printf("quick_read: %d\n", ret);
    if (ret <= 0)
    {
        free(buf);
        return;
    }
    *(unsigned short*)buf = htons(ret);
    write_n(net_fd, buf, ret + sizeof(want_len));
    free(buf);
    printf("do_tun\n");
}

static void do_net(int net_fd, int tun_fd)
{
    char* buf;
    unsigned short dst_len;
    ssize_t ret;
    
    ret = read_n(net_fd, &dst_len, sizeof(dst_len));
    printf("ret1: %d\n", ret);
    if (ret <= 0) return;
    dst_len = ntohs(dst_len);
    dst_len = 88;
    printf("dst_len: %u\n", dst_len);
    buf = malloc(dst_len);
    ret = read_n(net_fd, buf, dst_len);
    printf("ret2: %d\n", ret);
    if (ret <= 0)
    {
        free(buf);
        return;
    }
    write_n(tun_fd, buf, dst_len);
    free(buf);
    printf("do_net\n");
}

void do_network(int count, fd_set* set)
{
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
            do_net(network->connfd, conf.tun_fd);
        }
        if (FD_ISSET(conf.tun_fd, set)) // 服务器有数据
        {
            do_tun(conf.tun_fd, network->connfd);
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
        client_network_t* network = network_this;
        if (FD_ISSET(conf.tun_fd, set)) // 客户端有数据
        {
            do_tun(conf.tun_fd, network->serverfd);
        }
        if (FD_ISSET(network->serverfd, set)) // 服务器有数据
        {
            do_net(network->serverfd, conf.tun_fd);
        }
    }
}

