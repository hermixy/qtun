#include <arpa/inet.h>
#include <linux/icmp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "network.h"

static int connfd = -1;
static unsigned int client_id = 0;

int bind_and_listen(unsigned short port)
{
    int fd, rc;
    int opt = 1;
    struct sockaddr_in addr = {0};

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

    return fd;
}

int connect_server(char* ip, unsigned short port)
{
    int fd, rc;
    struct sockaddr_in addr = {0};
    unsigned char buffer[1024] = {0};
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
        strcpy(buffer, CLIENT_AUTH_MSG);
        client_id = pid;
        client_id = htonl(client_id);
        memcpy(&buffer[sizeof(CLIENT_AUTH_MSG) - sizeof(client_id) - 1], &client_id, sizeof(client_id));
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

void reply_echo(int fd, struct iphdr* ipHdr)
{
    unsigned char ipHdrLen = ipHdr->ihl << 2;
    struct icmphdr* icmpHdr = (struct icmphdr*)((char*)ipHdr + ipHdrLen); // 跳过IP头和可选头
    unsigned short ipLen = ntohs(ipHdr->tot_len) - ipHdrLen;

    __be32 tmp = ipHdr->saddr;
    ipHdr->saddr = ipHdr->daddr;
    ipHdr->daddr = tmp;
    ipHdr->check = 0;
    ipHdr->check = checksum(ipHdr, ipHdrLen);

    icmpHdr->type = ICMP_ECHOREPLY;
    icmpHdr->checksum = 0;
    icmpHdr->checksum = checksum(icmpHdr, ipLen);

    write_n(fd, ipHdr, ipHdrLen + ipLen);
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
        memcpy(&client_id, &buffer[sizeof(CLIENT_AUTH_MSG) - sizeof(client_id) - 2], sizeof(client_id));
        client_id = ntohl(client_id);
        connfd = fd;
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
    if (FD_ISSET(remotefd, set))
    {
        if (connfd == -1) accept_and_check(remotefd);
        else
        {
            int fd = accept(remotefd, NULL, NULL);
            if (fd != -1)
            {
                fprintf(stderr, "I can only process 1 client!\n");
                close(fd);
            }
        }
    }
    if (connfd != -1 && FD_ISSET(connfd, set))
    {
        unsigned char buffer[1024] = {0};
        ssize_t readen = read(connfd, buffer, sizeof(buffer));
        int reply = 0;
        if (readen > 0)
        {
            struct iphdr* ipHdr = (struct iphdr*)buffer;
            if (ipHdr->version == 4 && ipHdr->protocol == IPPROTO_ICMP) // 只处理ICMP的IPV4包
            {
                unsigned char ipHdrLen = ipHdr->ihl << 2;
                struct icmphdr* icmpHdr = (struct icmphdr*)(buffer + ipHdrLen); // 跳过IP头和可选头
                if (icmpHdr->type == ICMP_ECHO) // 只处理ECHO包
                {
                    reply_echo(connfd, ipHdr);
                    reply = 1;
                }
            }
            if (!reply)
            {
                ssize_t i;
                printf("received:\n");
                for (i = 0; i < readen; ++i)
                {
                    printf("%02X ", buffer[i]);
                }
                printf("\n");
            }
        }
        else
        {
            close(connfd);
            connfd = -1;
        }
    }
}

static void client_process(int max, fd_set* set, int remotefd, int localfd)
{
    unsigned char buffer[1024] = {0};
    ssize_t readen;
    if (FD_ISSET(localfd, set))
    {
        readen = read(localfd, buffer, sizeof(buffer));
        if (readen > 0) write_n(remotefd, buffer, readen);
    }
    if (FD_ISSET(remotefd, set))
    {
        readen = read(remotefd, buffer, sizeof(buffer));
        if (readen > 0) write_n(localfd, buffer, readen);
    }
}

void server_loop(int remotefd, int localfd)
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
        if (connfd != -1)
        {
            FD_SET(connfd, &set);
            if (connfd > max) max = connfd;
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
