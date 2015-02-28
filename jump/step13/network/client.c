#ifndef WIN32
#include <arpa/inet.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#endif
#include <errno.h>
#ifndef WIN32
#include <netdb.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "common.h"
#include "library.h"

#include "network.h"

#define RETURN_OK                 0
#define RETURN_CONNECTION_CLOSED -1
#define RETURN_READ_ERROR        -2

int connect_server(char* host, unsigned short port)
{
    fd_type fd;
    int rc;
    struct hostent* he;
    struct sockaddr_in addr = {0};
    msg_t* msg;
    char flag = 1;

    this.client.fd = this.remotefd = fd = socket(AF_INET, this.use_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ((he = gethostbyname(host)) == NULL)
    {
        SYSLOG(LOG_ERR, "Convert ip address error");
        close(fd);
        return -1;
    }
    addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];

    this.client.remote_ip = addr.sin_addr.s_addr;
    this.client.addr = addr;

    rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1)
    {
        perror("connect");
        close(fd);
        return -1;
    }

    if (!this.use_udp && setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
    {
        perror("setsockopt");
    }

    msg = new_login_msg(this.localip, 0, 0, 1);
    if (msg)
    {
        write_c(&this.client, msg, sizeof(msg_t) + msg_data_length(msg));
        pool_room_free(&this.pool, MSG_ROOM_IDX);
        if (read_msg_t(&this.client, &msg, 5) > 0)
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
            unsigned int gateway;
            unsigned short internal_mtu;
#ifdef WIN32
            char cmd[1024];
            char str[16];
#endif
            struct in_addr a;
            if (msg->compress != this.compress || msg->encrypt != this.encrypt)
            {
                SYSLOG(LOG_ERR, "compress algorithm or encrypt algorithm is not same");
                pool_room_free(&this.pool, RECV_ROOM_IDX);
                goto end;
            }
            if (!parse_login_reply_msg(msg, &ip, &gateway, &mask, &internal_mtu)) goto end;
            pool_room_free(&this.pool, RECV_ROOM_IDX);
            if (ip == 0)
            {
                SYSLOG(LOG_ERR, "Not enough ip address");
                goto end;
            }
            if (ip != this.localip)
            {
                char saddr[16], daddr[16];
                a.s_addr = this.localip;
                strcpy(saddr, inet_ntoa(a));
                a.s_addr = ip;
                strcpy(daddr, inet_ntoa(a));
                SYSLOG(LOG_ERR, "%s is inuse, but %s is not inuse", saddr, daddr);
                goto end;
            }
            hash_init(&this.client.recv_table, functor, 11);
            this.client.local_ip = gateway;
            this.client.fd = fd;
            this.client.internal_mtu = ntohs(internal_mtu);
            this.client.max_length = ROUND_UP(this.client.internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - (this.use_udp ? sizeof(struct udphdr) : sizeof(struct tcphdr)), 8);
            if (this.use_udp)
            {
                this.recv_buffer_len = this.client.max_length + sizeof(msg_t);
                this.recv_buffer = pool_room_realloc(&this.pool, RECV_ROOM_IDX, this.recv_buffer_len);
                if (this.recv_buffer == NULL)
                {
                    SYSLOG(LOG_INFO, "Not enough memory");
                    goto end;
                }
            }
            this.netmask = mask;
            this.keepalive = (unsigned int)time(NULL);
#ifdef WIN32 // 将对端内网IP添加到ARP表
            a.s_addr = gateway;
            strcpy(str, inet_ntoa(a));
            sprintf(cmd, "netsh -c \"i i\" add neighbors 17 %s ff-ff-ff-ff-ff-ff", str); // TODO: find interface ID
            SYSTEM_NORMAL(cmd);
#endif
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

static void process_msg(msg_t* msg)
{
    void* buffer = NULL;
    unsigned short len;
    int sys;
    size_t room_id;

    if (!check_msg(&this.client, msg)) return;

    if (msg->syscontrol)
    {
        client_process_sys(msg);
    }
    else if (msg->zone.clip)
    {
        if (!process_clip_msg(this.localfd, &this.client, msg, &room_id)) goto end;
    }
    else if (parse_msg(msg, &sys, &buffer, &len, &room_id))
    {
        ssize_t written;
#ifdef WIN32
        WriteFile(this.localfd, buffer, len, &written, NULL);
#else
        written = write(this.localfd, buffer, len);
#endif
        SYSLOG(LOG_INFO, "write local length: %ld", written);
    }
    else
        SYSLOG(LOG_WARNING, "Parse message error");
end:
    if (buffer) pool_room_free(&this.pool, room_id);
    if (!this.use_udp)
    {
        this.client.status = (this.client.status & ~CLIENT_STATUS_WAITING_BODY) | CLIENT_STATUS_WAITING_HEADER;
        this.client.want = sizeof(msg_t);
        this.client.read = this.client.buffer;
        this.client.buffer_len = this.client.want;
    }
    ++this.msg_ttl;
}

static int client_process(int max, fd_set* set)
{
    msg_group_t* group;
#ifdef WIN32
    if (local_have_data())
#else
    if (FD_ISSET(this.localfd, set))
#endif
    {
        unsigned char buffer[2048];
        ssize_t readen;

#ifdef WIN32
        ReadFile(this.localfd, buffer, sizeof(buffer), &readen, NULL);
#else
        readen = read(this.localfd, buffer, sizeof(buffer));
#endif
        if (readen > 0)
        {
            group = new_msg_group(buffer, (unsigned short)readen);
            if (group)
            {
                ssize_t written = send_msg_group(&this.client, group);
                msg_group_free(group);
                SYSLOG(LOG_INFO, "send msg length: %ld", written);
            }
        }
    }
    if (FD_ISSET(this.remotefd, set))
    {
        ssize_t rc = this.use_udp ? udp_read(this.remotefd, this.recv_buffer, this.recv_buffer_len, NULL, NULL)
                                  : read_pre(this.remotefd, this.client.read, this.client.want);
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
        else if (this.use_udp) // use udp
        {
            process_msg((msg_t*)this.recv_buffer);
        }
        else // use tcp
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
                            if (msg->zone.last) this.client.want = len % this.client.max_length;
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
                    else process_msg((msg_t*)this.client.buffer);
                }
                else process_msg((msg_t*)this.client.buffer);
            }
        }
    }
    return RETURN_OK;
}

void client_loop(
    fd_type remotefd,
#ifdef WIN32
    HANDLE localfd
#else
    int localfd
#endif
)
{
    fd_set set;
    int max;
    int keepalive_send = 0;
    int rc;

    this.remotefd = remotefd;
    this.localfd = localfd;
    if (!this.use_udp)
    {
        this.client.status = CLIENT_STATUS_NORMAL | CLIENT_STATUS_WAITING_HEADER;
        this.client.want = sizeof(msg_t);
        this.client.buffer = this.client.read = pool_room_realloc(&this.pool, RECV_ROOM_IDX, this.client.want);
        this.client.buffer_len = this.client.want;
    }

    this.keepalive_replyed = 1;
    while (1)
    {
        struct timeval tv = {0, 1};
        FD_ZERO(&set);
        FD_SET(remotefd, &set);
#ifdef WIN32
        max = remotefd;
#else
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;
#endif

        if (this.keepalive_replyed && (time(NULL) - this.keepalive) > KEEPALIVE_INTERVAL)
        {
            msg_t* msg = new_keepalive_msg(1);
            write_c(&this.client, msg, sizeof(msg_t));
            SYSLOG(LOG_INFO, "send keepalive message");
            this.keepalive = (unsigned int)time(NULL);
            this.keepalive_replyed = 0;
            pool_room_free(&this.pool, MSG_ROOM_IDX);
            keepalive_send = 1;
        }

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max >= 0)
        {
            rc = client_process(max, &set);
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
        checkout_ttl(&this.client.recv_table);
    }
}

