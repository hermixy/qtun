#include <arpa/inet.h>
#include <linux/icmp.h>
#include <linux/if_tun.h>
#include <linux/ip.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "msg.h"
#include "network.h"

network_t network;

static ssize_t read_msg(int fd, msg_t** msg)
{
    ssize_t rc;
    size_t len;

    *msg = malloc(sizeof(msg_t));
    if (*msg == NULL) return -2;
    rc = read_n(fd, *msg, sizeof(**msg));
    if (rc <= 0) return rc;
    len = msg_data_length(*msg);
    *msg = realloc(*msg, sizeof(msg_t) + len);
    if (*msg == NULL) return -2;
    rc = read_n(fd, (*msg)->data, len);

    printf("read msg length: %lu\n", len);
    return rc;
}

int tun_open(char name[IFNAMSIZ])
{
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1)
    {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) == -1)
    {
        perror("ioctl");
        close(fd);
        return -1;
    }

    strncpy(name, ifr.ifr_name, IFNAMSIZ);
    return fd;
}

int bind_and_listen(unsigned short port)
{
    int fd, rc;
    int opt = 1;
    struct sockaddr_in addr = {0};
    hash_functor_t functor_fd = {
        fd_hash,
        fd_compare,
        fd_dup,
        hash_dummy_dup,
        hash_dummy_free,
        NULL
    };
    hash_functor_t functor_ip = {
        ip_hash,
        ip_compare,
        ip_dup,
        hash_dummy_dup,
        hash_dummy_free,
        NULL
    };

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (rc == -1)
    {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1)
    {
        perror("bind");
        close(fd);
        return -1;
    }

    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    hash_init(&network.server.hash_fd, functor_fd, 11);
    hash_init(&network.server.hash_ip, functor_ip, 11);
    return fd;
}

int connect_server(char* ip, unsigned short port)
{
    int fd, rc;
    struct sockaddr_in addr = {0};
    char buffer[1024] = {0};
    ssize_t readen;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip, &addr.sin_addr) == 0)
    {
        fprintf(stderr, "Convert ip address error!\n");
        close(fd);
        return -1;
    }

    rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1)
    {
        perror("connect");
        close(fd);
        return -1;
    }

    readen = read_t(fd, buffer, sizeof(buffer), 5);
    if (readen > 0 && strcmp(SERVER_AUTH_MSG, buffer) == 0)
    {
        pid_t pid = getpid();
        unsigned int cid;
        strcpy(buffer, CLIENT_AUTH_MSG);
        cid = htonl(pid);
        network.client.id = pid;
        network.client.fd = fd;
        memcpy(&buffer[sizeof(CLIENT_AUTH_MSG) - sizeof(cid) - 1], &cid, sizeof(cid));
        write_n(fd, buffer, sizeof(CLIENT_AUTH_MSG) - 1);
    }
    else
    {
        fprintf(stderr, "is not allowed server\n");
        close(fd);
        return -1;
    }

    return fd;
}

static void accept_and_check(int bindfd)
{
    int fd = accept(bindfd, NULL, NULL);
    char buffer[sizeof(CLIENT_AUTH_MSG) - 1];
    ssize_t readen;
    if (fd == -1) return;

    write_n(fd, SERVER_AUTH_MSG, sizeof(SERVER_AUTH_MSG) - 1);
    readen = read_t(fd, buffer, sizeof(buffer), 5);
    if (readen <= 0)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        char* str;

        if (getpeername(fd, (struct sockaddr*)&addr, &len) == -1)
        {
            perror("getpeername");
            close(fd);
            return;
        }
        str = inet_ntoa(addr.sin_addr);
        fprintf(stderr, "authcheck failed: %s\n", str);
        close(fd);
        return;
    }

    if (strncmp(buffer, CLIENT_AUTH_MSG, sizeof(CLIENT_AUTH_MSG) - 5) == 0)
    {
        client_t* client = malloc(sizeof(*client));
        if (client == NULL)
        {
            fprintf(stderr, "out of memory\n");
            close(fd);
            return;
        }
        client->id = ntohl(*(unsigned int*)&buffer[sizeof(CLIENT_AUTH_MSG) - sizeof(client->id) - 2]);
        client->fd = fd;
        if (!hash_set(&network.server.hash_fd, (void*)(long)fd, sizeof(fd), client, sizeof(*client)))
        {
            fprintf(stderr, "hash set error\n");
            close(fd);
            return;
        }
    }
    else
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        char* str;

        if (getpeername(fd, (struct sockaddr*)&addr, &len) == -1)
        {
            perror("getpeername");
            close(fd);
            return;
        }
        str = inet_ntoa(addr.sin_addr);
        fprintf(stderr, "authcheck failed: %s\n", str);
        close(fd);
    }
}

static void server_process(int max, fd_set* set, int remotefd, int localfd)
{
    msg_t* msg;
    hash_iterator_t iter;
    struct iphdr* ipHdr;
    void* value;
    size_t value_len;

    if (FD_ISSET(remotefd, set)) accept_and_check(remotefd);
    iter = hash_begin(&network.server.hash_fd);
    while (!hash_is_end(iter))
    {
        void* buffer;
        unsigned short len;
        int sys;
        int fd = hash2fd(iter.data.key);
        if (FD_ISSET(fd, set))
        {
            if (read_msg(fd, &msg) > 0 && parse_msg(msg, &sys, &buffer, &len))
            {
                ipHdr = (struct iphdr*)buffer;
                if (hash_get(&network.server.hash_ip, (void*)(long)ipHdr->daddr, sizeof(ipHdr->daddr), &value, &value_len)) // 是本地局域网的则直接转走
                {
                    write_n(hash2fd(value), msg, sizeof(msg_t) + msg_data_length(msg));
                    printf("send msg length: %lu\n", msg_data_length(msg));
                }
                else
                {
                    if (sys) ;
                    else printf("write local length: %ld\n", write_n(localfd, buffer, len));
                }
                hash_set(&network.server.hash_ip, (void*)(long)ipHdr->saddr, sizeof(ipHdr->saddr), iter.data.key, iter.data.key_len);
            }
            else
            {
                close(fd);
                hash_del(&network.server.hash_fd, iter.data.key, iter.data.key_len);
            }
        }
        iter = hash_next(&network.server.hash_fd, iter);
    }
    if (FD_ISSET(localfd, set))
    {
        unsigned char buffer[2048];
        ssize_t readen;

        readen = read(localfd, buffer, sizeof(buffer));
        if (readen > 0)
        {
            ipHdr = (struct iphdr*)buffer;
            if (hash_get(&network.server.hash_ip, (void*)(long)ipHdr->daddr, sizeof(ipHdr->daddr), &value, &value_len))
            {
                msg = new_msg(buffer, readen);
                if (msg)
                {
                    write_n(hash2fd(value), msg, sizeof(msg_t) + msg_data_length(msg));
                    printf("send msg length: %lu\n", msg_data_length(msg));
                }
            }
        }
    }
}

static void client_process(int max, fd_set* set, int remotefd, int localfd)
{
    msg_t* msg;
    if (FD_ISSET(localfd, set))
    {
        unsigned char buffer[2048];
        ssize_t readen;

        readen = read(localfd, buffer, sizeof(buffer));
        if (readen > 0)
        {
            msg = new_msg(buffer, readen);
            if (msg)
            {
                write_n(remotefd, msg, sizeof(msg_t) + msg_data_length(msg));
                printf("send msg length: %lu\n", msg_data_length(msg));
            }
        }
    }
    if (FD_ISSET(remotefd, set))
    {
        int sys;
        void* buffer;
        unsigned short len;

        if (read_msg(remotefd, &msg) > 0 && parse_msg(msg, &sys, &buffer, &len))
        {
            if (sys) ;
            else printf("write local length: %ld\n", write_n(localfd, buffer, len));
        }
        else
        {
            fprintf(stderr, "read error\n");
            close(remotefd);
            exit(1);
        }
    }
}

void server_loop(int remotefd, int localfd)
{
    fd_set set;
    int max;
    while (1)
    {
        struct timeval tv = {60, 0};
        hash_iterator_t iter;

        FD_ZERO(&set);
        FD_SET(remotefd, &set);
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;
        iter = hash_begin(&network.server.hash_fd);
        while (!hash_is_end(iter))
        {
            int fd = hash2fd(iter.data.key);
            FD_SET(fd, &set);
            if (fd > max) max = fd;
            iter = hash_next(&network.server.hash_fd, iter);
        }

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max > 0) server_process(max, &set, remotefd, localfd);
    }
}

void client_loop(int remotefd, int localfd)
{
    fd_set set;
    int max;
    while (1)
    {
        struct timeval tv = {60, 0};
        FD_ZERO(&set);
        FD_SET(remotefd, &set);
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max > 0) client_process(max, &set, remotefd, localfd);
    }
}

ssize_t read_n(int fd, void* buf, size_t count)
{
    char* ptr = buf;
    size_t left = count;
    while (left)
    {
        ssize_t readen = read(fd, ptr, left);
        if (readen == 0) return 0;
        else if (readen == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }
        ptr  += readen;
        left -= readen;
    }
    return count;
}

ssize_t write_n(int fd, const void* buf, size_t count)
{
    const char* ptr = buf;
    size_t left = count;
    while (left)
    {
        ssize_t written = write(fd, ptr, left);
        if (written == 0) return 0;
        else if (written == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }
        ptr  += written;
        left -= written;
    }
    return count;
}

ssize_t read_t(int fd, void* buf, size_t count, double timeout)
{
    fd_set set;
    struct timeval tv = {(long)timeout, (long)(timeout * 1000000) % 1000000};
    int rc;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    rc = select(fd + 1, &set, NULL, NULL, &tv);
    switch (rc)
    {
    case -1:
        return rc;
    case 0:
        errno = EAGAIN;
        return -1;
    default:
        return read(fd, buf, count);
    }
}
