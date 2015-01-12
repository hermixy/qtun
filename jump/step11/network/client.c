#include <arpa/inet.h>
#include <linux/ip.h>
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
        SYSLOG(LOG_ERR, "Convert ip address error!");
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
        pool_room_free(&this.pool, MSG_ROOM_IDX);
        if (read_msg_t(fd, &msg, 5) > 0)
        {
            unsigned int ip;
            unsigned char mask;
            hash_functor_t functor = {
                msg_ident_hash,
                msg_ident_compare,
                hash_dummy_dup,
                hash_dummy_dup,
                msg_group_free_hash,
                msg_group_free_hash_val
            };
            unsigned short internal_mtu;
            if (msg->compress != this.compress || msg->encrypt != this.encrypt)
            {
                SYSLOG(LOG_ERR, "compress algorithm or encrypt algorithm is not same");
                pool_room_free(&this.pool, RECV_ROOM_IDX);
                goto end;
            }
            if (!parse_login_reply_msg(msg, &ip, &mask, &internal_mtu)) goto end;
            pool_room_free(&this.pool, RECV_ROOM_IDX);
            if (ip == 0)
            {
                SYSLOG(LOG_ERR, "Not enough ip address");
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
                SYSLOG(LOG_ERR, "%s is inuse, but %s is not inuse", saddr, daddr);
                goto end;
            }
            hash_init(&this.client.recv_table, functor, 11);
            this.client.internal_mtu = ntohs(internal_mtu);
            this.client.max_length = this.client.internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - sizeof(struct tcphdr);
            this.netmask = mask;
            this.keepalive = time(NULL);
            return fd;
        }
        SYSLOG(LOG_ERR, "read sys_login_reply message timeouted");
        goto end;
    }
    SYSLOG(LOG_ERR, "Can not create login message");
end:
    close(fd);
    return -1;
}

static void client_process_sys(msg_t* msg)
{
    switch (GET_SYS_OP(msg->sys))
    {
    case SYS_PING:
        if (IS_SYS_REPLY(msg->sys))
        {
            this.keepalive_replyed = 1;
            SYSLOG(LOG_INFO, "reply keepalive message received");
        }
        break;
    }
}

static void process_msg(msg_t* msg, int localfd)
{
    void* buffer = NULL;
    unsigned short len;
    int sys;
    size_t room_id;

    if (msg->syscontrol)
    {
        client_process_sys(msg);
    }
    else if (msg->zone.clip)
    {
        if (!process_clip_msg(localfd, &this.client, msg, &room_id)) goto end;
    }
    else if (parse_msg(msg, &sys, &buffer, &len, &room_id))
    {
        ssize_t written = write_n(localfd, buffer, len);
        SYSLOG(LOG_INFO, "write local length: %ld", written);
    }
    else
        SYSLOG(LOG_WARNING, "Parse message error");
end:
    if (buffer) pool_room_free(&this.pool, room_id);
    this.client.status = (this.client.status & ~CLIENT_STATUS_WAITING_BODY) | CLIENT_STATUS_WAITING_HEADER;
    this.client.want = sizeof(msg_t);
    this.client.read = this.client.buffer;
    this.client.buffer_len = this.client.want;
    ++this.msg_ttl;
}

static int client_process(int max, fd_set* set, int remotefd, int localfd)
{
    msg_group_t* group;
    if (FD_ISSET(localfd, set))
    {
        unsigned char buffer[2048];
        ssize_t readen;

        readen = read(localfd, buffer, sizeof(buffer));
        if (readen > 0)
        {
            group = new_msg_group(buffer, readen);
            if (group)
            {
                ssize_t written = send_msg_group(remotefd, group);
                msg_group_free(group);
                SYSLOG(LOG_INFO, "send msg length: %ld", written);
            }
        }
    }
    if (FD_ISSET(remotefd, set))
    {
        ssize_t rc = read_pre(remotefd, this.client.read, this.client.want);
        if (rc == 0)
        {
            SYSLOG(LOG_ERR, "connection closed");
            return RETURN_CONNECTION_CLOSED;
        }
        else if (rc < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return RETURN_OK;
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
                        msg_t* msg = (msg_t*)this.client.buffer;
                        this.client.status = (this.client.status & ~CLIENT_STATUS_WAITING_HEADER) | CLIENT_STATUS_WAITING_BODY;
                        if (msg->zone.clip)
                        {
                            if (msg->zone.last) this.client.want = len & this.client.max_length;
                            else this.client.want = this.client.max_length;
                        }
                        else this.client.want = len;
                        this.client.buffer_len = sizeof(msg_t) + this.client.want;
                        this.client.buffer = pool_room_realloc(&this.pool, RECV_ROOM_IDX, this.client.buffer_len);
                        if (this.client.buffer == NULL)
                        {
                            SYSLOG(LOG_ERR, "Not enough memory");
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
    this.client.buffer = this.client.read = pool_room_alloc(&this.pool, RECV_ROOM_IDX, this.client.want);
    this.client.buffer_len = this.client.want;
    int keepalive_send = 0;
    int rc;
    if (this.client.buffer == NULL)
    {
        SYSLOG(LOG_ERR, "Not enough memory");
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
            SYSLOG(LOG_INFO, "send keepalive message");
            this.keepalive = time(NULL);
            this.keepalive_replyed = 0;
            pool_room_free(&this.pool, MSG_ROOM_IDX);
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
                pool_room_free(&this.pool, RECV_ROOM_IDX);
                return;
            }
        }

        if (keepalive_send && !this.keepalive_replyed && (time(NULL) - this.keepalive) > KEEPALIVE_TIMEOUT)
        {
            SYSLOG(LOG_INFO, "keepalive reply timeouted, connection closed");
            pool_room_free(&this.pool, RECV_ROOM_IDX);
            return;
        }
        // TODO: 扫垃圾，将expired group清除
    }
}

