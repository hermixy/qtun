#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if defined(unix) && defined(HAVE_LINUX_TCP_H)
#include <linux/tcp.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "library.h"
#include "proto.h"
#include "vector.h"

#include "network.h"

int bind_and_listen(unsigned short port)
{
    int fd, rc;
#ifdef WIN32
    char opt = 1;
#else
    int opt = 1;
#endif
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);

    this.remotefd = fd = (int)socket(AF_INET, this.use_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
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

    if (this.use_udp) return fd;

    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static int check_ip_by_mask(unsigned int src, unsigned int dst, unsigned char mask)
{
    unsigned int m = LEN2MASK(mask);
    return (src & m) == (dst & m);
}

static void accept_and_check()
{
    struct sockaddr_in srcaddr;
    socklen_t addrlen = sizeof(srcaddr);
    int fd = (int)accept(this.remotefd, (struct sockaddr*)&srcaddr, &addrlen);
    client_t* client;
#ifdef WIN32
    char flag = 1;
#else
    int flag = 1;
#endif
    hash_functor_t functor = {
        msg_ident_hash,
        msg_ident_compare,
        hash_dummy_dup,
        hash_dummy_dup,
        msg_group_free_hash,
        msg_group_free_hash_val
    };
    if (fd == -1) return;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
    {
        perror("setsockopt");
    }

    client = malloc(sizeof(*client));
    if (client == NULL)
    {
        SYSLOG(LOG_ERR, "Not enough memory");
        close(fd);
        return;
    }

    client->remote_ip = srcaddr.sin_addr.s_addr;
    client->fd = fd;
    client->addr = srcaddr;
    client->keepalive = (unsigned int)time(NULL);
    client->status = CLIENT_STATUS_CHECKLOGIN | CLIENT_STATUS_WAITING_HEADER;
    client->want = sizeof(msg_t);
    client->buffer = client->read = group_pool_room_alloc(&this.group_pool, client->want);
    client->buffer_len = client->want;
    if (client->buffer == NULL)
    {
        SYSLOG(LOG_ERR, "Not enough memory");
        free(client);
        close(fd);
        return;
    }
    if (!hash_init(&client->recv_table, functor, 11))
    {
        SYSLOG(LOG_ERR, "hash_init failed");
        group_pool_room_free(&this.group_pool, client->buffer);
        free(client);
        close(fd);
        return;
    }
    if (!active_vector_append(&this.clients, client, sizeof(*client)))
    {
        SYSLOG(LOG_ERR, "append to clients error");
        free(client);
        close(fd);
    }
}

static void remove_clients(vector_t* v, const char* pfx)
{
    void* tmp;
    size_t tmp_len;
    while (vector_pop_back(v, &tmp, &tmp_len))
    {
        client_t* client;
        char ip[16];
        struct in_addr a;
        active_vector_get(&this.clients, (size_t)tmp, (void**)&client, &tmp_len);
        a.s_addr = client->remote_ip;
        sprintf(ip, "%s", inet_ntoa(a));
        SYSLOG(LOG_INFO, "%s: %s", pfx, ip);
        close(client->fd);
        hash_free(&client->recv_table);
        active_vector_del(&this.clients, (size_t)tmp);
    }
}

static void close_client(vector_t* for_del, size_t idx)
{
    vector_push_back(for_del, (void*)(long)idx, sizeof(idx));
}

static void server_process_sys(client_t* client, msg_t* msg)
{
    switch (GET_SYS_OP(msg->sys))
    {
    case SYS_PING:
        if (IS_SYS_REQUEST(msg->sys))
        {
            client->keepalive = (unsigned int)time(NULL);
            msg_t* new_msg = new_keepalive_msg(0);
            write_c(client, new_msg, sizeof(msg_t));
            SYSLOG(LOG_INFO, "reply keepalive message");
            pool_room_free(&this.pool, MSG_ROOM_IDX);
        }
        break;
    }
}

static void server_process_login(client_t* client, msg_t* msg, size_t idx, vector_t* for_del)
{
    sys_login_msg_t* login;
    msg_t* new_msg;
    int sys;
    void* data = NULL;
    unsigned short len;
    size_t room_id;

    if (!IS_CLIENT_STATUS_CHECKLOGIN(client->status))
    {
        SYSLOG(LOG_ERR, "Invalid status, want(%d) current(%d)", CLIENT_STATUS_CHECKLOGIN, client->status);
        close_client(for_del, idx);
        goto end;
    }
    if (msg->compress != this.compress || msg->encrypt != this.encrypt) // 算法不同直接将本地的加密压缩算法返回
    {
        msg->compress = this.compress;
        msg->encrypt = this.encrypt;
        msg->checksum = 0;
        msg->checksum = checksum(msg, sizeof(msg_t) + msg_data_length(msg));
        write_c(client, msg, sizeof(msg_t) + msg_data_length(msg));
        goto end;
    }
    if (!parse_msg(msg, &sys, &data, &len, &room_id))
    {
        SYSLOG(LOG_ERR, "parse sys_login_request failed");
        close_client(for_del, idx);
        goto end;
    }
    login = (sys_login_msg_t*)data;
    if (memcmp(login->check, SYS_MSG_CHECK, sizeof(login->check)) ||
        !check_ip_by_mask(login->ip, this.localip, this.netmask)) // 非法数据包
    {
        SYSLOG(LOG_ERR, "unknown sys_login_request message");
        close_client(for_del, idx);
        goto end;
    }
    if (login->ip == this.localip || active_vector_exists(&this.clients, compare_clients_by_local_ip, (void*)(long)login->ip, sizeof(login->ip)) >= 0) // IP已被占用
    {
        unsigned short i;
        unsigned int localip = login->ip & LEN2MASK(this.netmask);
        for (i = 1; i < LEN2MASK(32 - this.netmask); ++i)
        {
            unsigned int newip = (i << this.netmask) | localip;
            if (active_vector_exists(&this.clients, compare_clients_by_local_ip, (void*)(long)newip, sizeof(newip)) == -1 && newip != this.localip)
            {
                pool_room_free(&this.pool, room_id);
                data = NULL;
                new_msg = new_login_msg(newip, 0, this.netmask, 0);
                if (new_msg)
                {
                    write_c(client, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
                    pool_room_free(&this.pool, MSG_ROOM_IDX);
                }
                else
                {
                    SYSLOG(LOG_ERR, "Can not create login message");
                    close_client(for_del, idx);
                }
                goto end;
            }
        }
        pool_room_free(&this.pool, room_id);
        data = NULL;
        new_msg = new_login_msg(0, 0, 0, 0);
        if (new_msg)
        {
            write_c(client, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
            pool_room_free(&this.pool, MSG_ROOM_IDX);
        }
        else
        {
            SYSLOG(LOG_ERR, "Can not create login message");
            close_client(for_del, idx);
        }
    }
    else
    {
        unsigned int remote_ip = login->ip;
        unsigned short internal_mtu = ntohs(login->internal_mtu);
        pool_room_free(&this.pool, room_id);
        data = NULL;
        new_msg = new_login_msg(remote_ip, this.localip, this.netmask, 0);
        if (new_msg == NULL)
        {
            SYSLOG(LOG_ERR, "Can not create login message");
            close_client(for_del, idx);
            goto end;
        }
        client->local_ip = remote_ip;
        client->keepalive = (unsigned int)time(NULL);
        client->internal_mtu = internal_mtu;
        client->max_length = ROUND_UP(client->internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - (this.use_udp ? sizeof(struct udphdr) : sizeof(struct tcphdr)), 8);
        client->status = CLIENT_STATUS_NORMAL;
        if (this.use_udp && client->max_length + sizeof(msg_t) > this.recv_buffer_len)
        {
            this.recv_buffer_len = client->max_length + sizeof(msg_t);
            this.recv_buffer = pool_room_realloc(&this.pool, RECV_ROOM_IDX, this.recv_buffer_len);
            if (this.recv_buffer == NULL)
            {
                SYSLOG(LOG_INFO, "Not enough memory");
                close_client(for_del, idx);
                goto end;
            }
        }
#ifdef WIN32 // 将对端内网IP添加到ARP表
        {
            char cmd[1024];
            char str[16];
            struct in_addr a;
            a.s_addr = remote_ip;
            strcpy(str, inet_ntoa(a));
            sprintf(cmd, "arp -s %s ff-ff-ff-ff-ff-ff", str); // TODO: get mac from remote
            SYSTEM_NORMAL(cmd);
        }
#endif
        write_c(client, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
        pool_room_free(&this.pool, MSG_ROOM_IDX);
    }
end:
    if (data) pool_room_free(&this.pool, room_id);
}

static void process_msg(client_t* client, msg_t* msg, vector_t* for_del, size_t idx)
{
    void* buffer = NULL;
    unsigned short len;
    int sys;
    size_t room_id;

    if (!check_msg(client, msg)) return;

    if (msg->syscontrol)
    {
        if (CHECK_SYS_OP(msg->sys, SYS_LOGIN, 1)) server_process_login(client, msg, idx, for_del);
        else
        {
            server_process_sys(client, msg);
            active_vector_up(&this.clients, idx);
        }
    }
    else if (msg->zone.clip)
    {
        if (!process_clip_msg(this.localfd, client, msg, &room_id)) goto end;
        active_vector_up(&this.clients, idx);
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
        active_vector_up(&this.clients, idx);
    }
    else
        SYSLOG(LOG_WARNING, "Parse message error");
end:
    if (buffer) pool_room_free(&this.pool, room_id);
    if (!this.use_udp)
    {
        client->status = (client->status & ~CLIENT_STATUS_WAITING_BODY) | CLIENT_STATUS_WAITING_HEADER;
        client->want = sizeof(msg_t);
        client->read = client->buffer;
        client->buffer_len = client->want;
    }
}

static void udp_process(vector_t* for_del)
{
    struct sockaddr_in srcaddr;
    socklen_t addrlen = sizeof(srcaddr);
    ssize_t rc;
    ssize_t idx;
    client_t* client = NULL;
    hash_functor_t functor = {
        msg_ident_hash,
        msg_ident_compare,
        hash_dummy_dup,
        hash_dummy_dup,
        msg_group_free_hash,
        msg_group_free_hash_val
    };

    rc = udp_read(this.remotefd, this.recv_buffer, this.recv_buffer_len, &srcaddr, &addrlen);
    if (rc <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        perror("recvfrom");
        return;
    }
    idx = active_vector_exists(&this.clients, compare_clients_by_remote_ip, (void*)(long)srcaddr.sin_addr.s_addr, sizeof(srcaddr.sin_addr.s_addr));
    if (idx >= 0)
    {
        size_t len;
        active_vector_get(&this.clients, idx, (void**)&client, &len);
        process_msg(client, (msg_t*)this.recv_buffer, for_del, idx);
    }
    else
    {
        msg_t* msg = (msg_t*)this.recv_buffer;
        if (msg->syscontrol && CHECK_SYS_OP(msg->sys, SYS_LOGIN, 1)) // 除了sys login其他一律丢包
        {
            client = malloc(sizeof(*client));
            if (client == NULL)
            {
                SYSLOG(LOG_ERR, "Not enough memory");
                return;
            }
            client->remote_ip = srcaddr.sin_addr.s_addr;
            client->addr = srcaddr;
            client->keepalive = (unsigned int)time(NULL);
            client->status = CLIENT_STATUS_CHECKLOGIN | CLIENT_STATUS_WAITING_BODY;
            if (!hash_init(&client->recv_table, functor, 11))
            {
                SYSLOG(LOG_ERR, "hash_init failed");
                free(client);
                return;
            }
            if (!active_vector_append(&this.clients, client, sizeof(*client)))
            {
                SYSLOG(LOG_ERR, "append to clients error");
                free(client);
            }
            process_msg(client, msg, for_del, active_vector_count(&this.clients));
        }
    }
    if (client) checkout_ttl(&client->recv_table);
}

static void tcp_process(fd_set* set, vector_t* for_del)
{
    active_vector_iterator_t iter = active_vector_begin(&this.clients);
    while (!active_vector_is_end(iter))
    {
        client_t* client = iter.data;
        if (FD_ISSET(client->fd, set))
        {
            ssize_t rc = read_pre(client->fd, client->read, client->want);
            if (rc <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) goto end;
                perror("read");
                vector_push_back(for_del, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
                goto end;
            }
            client->read += rc;
            client->want -= rc;
            if (client->want == 0)
            {
                if (IS_CLIENT_STATUS_WAITING_HEADER(client->status))
                {
                    size_t len = msg_data_length((msg_t*)client->buffer);
                    if (len)
                    {
                        msg_t* msg = (msg_t*)client->buffer;
                        client->status = (client->status & ~CLIENT_STATUS_WAITING_HEADER) | CLIENT_STATUS_WAITING_BODY;
                        if (msg->zone.clip)
                        {
                            if (msg->zone.last) client->want = len % client->max_length;
                            else client->want = client->max_length;
                        }
                        else client->want = len;
                        client->buffer_len = sizeof(msg_t) + client->want;
                        client->buffer = group_pool_room_realloc(&this.group_pool, client->buffer, sizeof(msg_t) + client->want);
                        if (client->buffer == NULL)
                        {
                            SYSLOG(LOG_ERR, "Not enough memory");
                            vector_push_back(for_del, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
                            exit(1);
                        }
                        client->read = ((msg_t*)client->buffer)->data;
                    }
                    else process_msg(client, (msg_t*)client->buffer, for_del, active_vector_iterator_idx(iter));
                }
                else process_msg(client, (msg_t*)client->buffer, for_del, active_vector_iterator_idx(iter));
            }
        }
        checkout_ttl(&client->recv_table);
end:
        iter = active_vector_next(iter);
    }
}

static void server_process(int max, fd_set* set)
{
    msg_group_t* group;
    struct iphdr* ipHdr;
    vector_t v;
    vector_functor_t f = {
        vector_dummy_dup,
        NULL
    };

    if (!this.use_udp && FD_ISSET(this.remotefd, set)) accept_and_check();
    vector_init(&v, f);
    if (this.use_udp)
    {
        if (FD_ISSET(this.remotefd, set)) udp_process(&v);
    }
    else tcp_process(set, &v);
    ++this.msg_ttl;
    remove_clients(&v, "closed");
    vector_free(&v);
    if (LOCAL_HAVE_DATA(set))
    {
        unsigned char buffer[2048];
        ssize_t readen = 0;

#ifdef WIN32
        ReadFile(this.localfd, buffer, sizeof(buffer), &readen, NULL);
#else
        readen = read(this.localfd, buffer, sizeof(buffer));
#endif
        if (readen > 0)
        {
            ssize_t idx;
            ipHdr = (struct iphdr*)buffer;
            idx = active_vector_lookup(&this.clients, compare_clients_by_local_ip, (void*)(long)ipHdr->daddr, sizeof(ipHdr->daddr));
            if (idx >= 0)
            {
                client_t* client;
                size_t len;
                active_vector_get(&this.clients, idx, (void**)&client, &len);
                group = new_msg_group(buffer, (unsigned short)readen);
                if (group)
                {
                    ssize_t written = send_msg_group(client, group);
                    msg_group_free(group);
                    SYSLOG(LOG_INFO, "send msg length: %ld", written);
                }
            }
        }
    }
}

void server_loop(fd_type remotefd, local_fd_type localfd)
{
    fd_set set;
    int max;
    vector_t v;
    vector_functor_t f = {
        vector_dummy_dup,
        NULL
    };

    this.remotefd = remotefd;
    this.localfd = localfd;

    vector_init(&v, f);
    while (1)
    {
        struct timeval tv = {0, 1};
        active_vector_iterator_t iter;

        FD_ZERO(&set);
        FD_SET(remotefd, &set);
#ifdef WIN32
        max = remotefd;
#else
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;
#endif
        if (!this.use_udp)
        {
            iter = active_vector_begin(&this.clients);
            while (!active_vector_is_end(iter))
            {
                int fd = ((client_t*)iter.data)->fd;
                FD_SET(fd, &set);
                if (fd > max) max = fd;
                iter = active_vector_next(iter);
            }
        }

        max = select(max + 1, &set, NULL, NULL, &tv);
        server_process(max, &set);

        iter = active_vector_begin(&this.clients);
        while (!active_vector_is_end(iter))
        {
            client_t* client = (client_t*)iter.data;
            if (IS_CLIENT_STATUS_CHECKLOGIN(client->status))
            {
                if ((time(NULL) - client->keepalive) > LOGIN_TIMEOUT)
                    vector_push_back(&v, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
            }
            else if (IS_CLIENT_STATUS_NORMAL(client->status))
            {
                if ((time(NULL) - ((client_t*)iter.data)->keepalive) > KEEPALIVE_LIMIT)
                    vector_push_back(&v, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
            }
            iter = active_vector_next(iter);
        }
        remove_clients(&v, "login or keepalive timeouted");
    }
    vector_free(&v);
}

