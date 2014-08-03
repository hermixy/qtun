#include <linux/if_tun.h>
#include <stdio.h>
#include <string.h>

#include "qvn.h"
#include "network.h"

#define PROXY_BLOCK_LENGTH  1024

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

static void accept_and_check(server_network_t* network)
{
    static char msg[] = "Hello";
    static char dst[sizeof(msg)] = {0};

    int rc;
    int fd = accept(network->bindfd, NULL, NULL);
    if (fd == -1) return;
    
    memset(dst, 0, sizeof(msg));
    rc = s_recv(fd, dst, sizeof(msg) - 1, 60);
    if (!rc) return;
    
    if (strcmp(msg, dst) == 0)
    {
        network->connfd = fd;
        // TODO: 打印IP地址等信息
    }
    else close(fd);
}

static void read_and_write(server_network_t* network, int from, int to)
{
    char* ptr = NULL;
    size_t len = 0;
    ssize_t readen;
    do
    {
        ptr = realloc(ptr, len + PROXY_BLOCK_LENGTH);
        readen = read(from, ptr + len, PROXY_BLOCK_LENGTH);
        if (readen == 0)
        {
            if (from == network->connfd) fprintf(stderr, "ERR: connection closed\n");
            else fprintf(stderr, "ERR: tun error\n");
            close(from);
            free(ptr);
            return;
        }
        else if (readen == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("read");
            free(ptr);
            return;
        }
        len += readen;
        if (readen < PROXY_BLOCK_LENGTH) break; // 已读完
    } while (1);
    s_send(to, ptr, 
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
                    s_send(fd, "error connection", sizeof("error connection") - 1);
                    close(fd);
                }
            }
        }
        if (network->connfd != -1 && FD_ISSET(network->connfd, set)) // 客户端有数据
        {
        }
        if (FD_ISSET(conf.tun_fd, set)) // 服务器有数据
        {
        }
    }
    else /* if (conf.type == QVN_CONF_TYPE_CLIENT) */
    {
    }
}

