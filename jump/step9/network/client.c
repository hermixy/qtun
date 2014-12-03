#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "library.h"

#include "network.h"

#define RETURN_OK                 0
#define RETURN_CONNECTION_CLOSED -1
#define RETURN_READ_ERROR        -2

int connect_server(char* ip, unsigned short port)
{
    int fd, rc;
    struct sockaddr_in addr = {0};
    msg_t* msg;
    int flag = 1;

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

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
    {
        perror("setsockopt");
    }

    msg = new_login_msg(this.localip, 0, 1);
    if (msg)
    {
        write_n(fd, msg, sizeof(msg_t) + msg_data_length(msg));
        free(msg);
        if (read_msg_t(fd, &msg, 5) > 0)
        {
            unsigned int ip;
            unsigned char mask;
            if (msg->compress != this.compress || msg->encrypt != this.encrypt)
            {
                fprintf(stderr, "compress algorithm or encrypt algorithm is not same\n");
                free(msg);
                goto end;
            }
            if (!parse_login_reply_msg(msg, &ip, &mask)) goto end;
            free(msg);
            if (ip == 0)
            {
                fprintf(stderr, "Not enough ip address\n");
                goto end;
            }
            if (ip != this.localip)
            {
                struct in_addr a;
                char saddr[16], daddr[16];
                a.s_addr = this.localip;
                strcpy(saddr, inet_ntoa(a));
                a.s_addr = ip;
                strcpy(daddr, inet_ntoa(a));
                fprintf(stderr, "%s is inuse, but %s is not inuse\n", saddr, daddr);
                goto end;
            }
            this.netmask = mask;
            this.keepalive = time(NULL);
            return fd;
        }
        fprintf(stderr, "read sys_login_reply message timeouted\n");
        goto end;
    }
    fprintf(stderr, "Not enough memory\n");
end:
    close(fd);
    return -1;
}

static void client_process_sys(msg_t* msg, const void* buffer, const size_t len)
{
    switch (GET_SYS_OP(msg->unused))
    {
    case SYS_PING:
        if (IS_SYS_REPLY(msg->unused))
        {
            this.keepalive_replyed = 1;
            printf("reply keepalive message received\n");
        }
        break;
    }
}

static void process_msg(msg_t* msg, int localfd)
{
    void* buffer = NULL;
    unsigned short len;
    int sys;

    if (parse_msg(msg, &sys, &buffer, &len))
    {
        if (sys) client_process_sys(msg, buffer, len);
        else printf("write local length: %ld\n", write_n(localfd, buffer, len));
    }
    else
    {
        fprintf(stderr, "Parse message error\n");
        return;
    }
    if (buffer) free(buffer);
    this.client.status = (this.client.status & ~CLIENT_STATUS_WAITING_BODY) | CLIENT_STATUS_WAITING_HEADER;
    this.client.want = sizeof(msg_t);
    this.client.read = this.client.buffer;
}

static int client_process(int max, fd_set* set, int remotefd, int localfd)
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
                free(msg);
                printf("send msg length: %lu\n", msg_data_length(msg));
            }
        }
    }
    if (FD_ISSET(remotefd, set))
    {
        ssize_t rc = read_pre(remotefd, this.client.read, this.client.want);
        if (rc == 0)
        {
            fprintf(stderr, "connection closed\n");
            return RETURN_CONNECTION_CLOSED;
        }
        else if (rc < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return RETURN_OK;
            fprintf(stderr, "read error\n");
            perror("read");
            return RETURN_READ_ERROR;
        }
        else
        {
            this.client.read += rc;
            this.client.want -= rc;
            if (this.client.want == 0)
            {
                if (IS_CLIENT_STATUS_WAITING_HEADER(this.client.status))
                {
                    size_t len = msg_data_length((msg_t*)this.client.buffer);
                    if (len)
                    {
                        this.client.status = (this.client.status & ~CLIENT_STATUS_WAITING_HEADER) | CLIENT_STATUS_WAITING_BODY;
                        this.client.want = len;
                        this.client.buffer = realloc(this.client.buffer, sizeof(msg_t) + this.client.want);
                        if (this.client.buffer == NULL)
                        {
                            fprintf(stderr, "Not enough memory\n");
                            exit(1);
                        }
                        this.client.read = ((msg_t*)this.client.buffer)->data;
                    }
                    else process_msg((msg_t*)this.client.buffer, localfd);
                }
                else process_msg((msg_t*)this.client.buffer, localfd);
            }
        }
    }
    return RETURN_OK;
}

void client_loop(int remotefd, int localfd)
{
    fd_set set;
    int max;
    this.client.status = CLIENT_STATUS_NORMAL | CLIENT_STATUS_WAITING_HEADER;
    this.client.want = sizeof(msg_t);
    this.client.buffer = this.client.read = malloc(this.client.want);
    int keepalive_send = 0;
    int rc;
    if (this.client.buffer == NULL)
    {
        fprintf(stderr, "Not enough memory\n");
        return;
    }
    this.keepalive_replyed = 1;
    while (1)
    {
        struct timeval tv = {1, 0};
        FD_ZERO(&set);
        FD_SET(remotefd, &set);
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;

        if (this.keepalive_replyed && (time(NULL) - this.keepalive) > KEEPALIVE_INTERVAL)
        {
            msg_t* msg = new_keepalive_msg(1);
            write_n(remotefd, msg, sizeof(msg_t));
            printf("send keepalive message\n");
            this.keepalive = time(NULL);
            this.keepalive_replyed = 0;
            free(msg);
            keepalive_send = 1;
        }

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max > 0)
        {
            rc = client_process(max, &set, remotefd, localfd);
            switch (rc)
            {
            case RETURN_CONNECTION_CLOSED:
            case RETURN_READ_ERROR:
                return;
            }
        }

        if (keepalive_send && !this.keepalive_replyed && (time(NULL) - this.keepalive) > KEEPALIVE_TIMEOUT)
        {
            fprintf(stderr, "keepalive reply timeouted, connection closed\n");
            return;
        }
    }
}

