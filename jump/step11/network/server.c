#include <arpa/inet.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "library.h"
#include "vector.h"

#include "network.h"

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

inline static int check_ip_by_mask(unsigned int src, unsigned int dst, unsigned char mask)
{
    unsigned int m = LEN2MASK(mask);
    return (src & m) == (dst & m);
}

static void accept_and_check(int bindfd)
{
    int fd = accept(bindfd, NULL, NULL);
    client_t* client;
    int flag = 1;
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

    client->fd = fd;
    client->keepalive = time(NULL);
    client->status = CLIENT_STATUS_CHECKLOGIN | CLIENT_STATUS_WAITING_HEADER;
    client->want = sizeof(msg_t);
    client->buffer = client->read = pool_room_realloc(&this.pool, RECV_ROOM_IDX, client->want);
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
        pool_room_free(&this.pool, RECV_ROOM_IDX);
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
        a.s_addr = client->ip;
        sprintf(ip, "%s", inet_ntoa(a));
        SYSLOG(LOG_INFO, "%s: %s", pfx, ip);
        close(client->fd);
        hash_free(&client->recv_table);
        active_vector_del(&this.clients, (size_t)tmp);
    }
}

inline static void close_client(vector_t* for_del, size_t idx)
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
            client->keepalive = time(NULL);
            msg_t* new_msg = new_keepalive_msg(0);
            write_n(client->fd, new_msg, sizeof(msg_t));
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
        write_n(client->fd, msg, sizeof(msg_t) + msg_data_length(msg));
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
    if (login->ip == this.localip || active_vector_exists(&this.clients, compare_clients_by_ip, (void*)(long)login->ip, sizeof(login->ip)) >= 0) // IP已被占用
    {
        unsigned short i;
        unsigned int localip = login->ip & LEN2MASK(this.netmask);
        for (i = 1; i < LEN2MASK(32 - this.netmask); ++i)
        {
            unsigned int newip = (i << this.netmask) | localip;
            if (active_vector_exists(&this.clients, compare_clients_by_ip, (void*)(long)newip, sizeof(newip)) == -1 && newip != this.localip)
            {
                pool_room_free(&this.pool, room_id);
                data = NULL;
                new_msg = new_login_msg(newip, this.netmask, 0);
                if (new_msg)
                {
                    write_n(client->fd, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
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
        new_msg = new_login_msg(0, 0, 0);
        if (new_msg)
        {
            write_n(client->fd, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
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
        new_msg = new_login_msg(remote_ip, this.netmask, 0);
        if (new_msg == NULL)
        {
            SYSLOG(LOG_ERR, "Can not create login message");
            close_client(for_del, idx);
            goto end;
        }
        client->ip = remote_ip;
        client->status = CLIENT_STATUS_NORMAL;
        client->keepalive = time(NULL);
        client->internal_mtu = internal_mtu;
        client->max_length = ROUND_UP(client->internal_mtu - sizeof(msg_t) - sizeof(struct iphdr) - sizeof(struct tcphdr), 8);
        write_n(client->fd, new_msg, sizeof(msg_t) + msg_data_length(new_msg));
        pool_room_free(&this.pool, MSG_ROOM_IDX);
    }
end:
    if (data) pool_room_free(&this.pool, room_id);
}

static void process_msg(client_t* client, msg_t* msg, int localfd, vector_t* for_del, size_t idx)
{
    void* buffer = NULL;
    unsigned short len;
    int sys;
    size_t room_id;

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
        if (!process_clip_msg(localfd, client, msg, &room_id)) goto end;
        active_vector_up(&this.clients, idx);
    }
    else if (parse_msg(msg, &sys, &buffer, &len, &room_id))
    {
        ssize_t written = write_n(localfd, buffer, len);
        SYSLOG(LOG_INFO, "write local length: %ld", written);
        active_vector_up(&this.clients, idx);
    }
    else
        SYSLOG(LOG_WARNING, "Parse message error");
end:
    if (buffer) pool_room_free(&this.pool, room_id);
    client->status = (client->status & ~CLIENT_STATUS_WAITING_BODY) | CLIENT_STATUS_WAITING_HEADER;
    client->want = sizeof(msg_t);
    client->read = client->buffer;
    client->buffer_len = client->want;
    ++this.msg_ttl;
}

static void server_process(int max, fd_set* set, int remotefd, int localfd)
{
    msg_group_t* group;
    active_vector_iterator_t iter;
    struct iphdr* ipHdr;
    vector_t v;
    vector_functor_t f = {
        vector_dummy_dup,
        NULL
    };

    if (FD_ISSET(remotefd, set)) accept_and_check(remotefd);
    iter = active_vector_begin(&this.clients);
    vector_init(&v, f);
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
                vector_push_back(&v, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
            }
            else
            {
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
                            client->buffer = pool_room_realloc(&this.pool, RECV_ROOM_IDX, sizeof(msg_t) + client->want);
                            if (client->buffer == NULL)
                            {
                                SYSLOG(LOG_ERR, "Not enough memory");
                                vector_push_back(&v, (void*)(long)active_vector_iterator_idx(iter), sizeof(active_vector_iterator_idx(iter)));
                                exit(1);
                            }
                            client->read = ((msg_t*)client->buffer)->data;
                        }
                        else process_msg(client, (msg_t*)client->buffer, localfd, &v, active_vector_iterator_idx(iter));
                    }
                    else process_msg(client, (msg_t*)client->buffer, localfd, &v, active_vector_iterator_idx(iter));
                }
            }
        }
        checkout_ttl(&client->recv_table);
end:
        iter = active_vector_next(iter);
    }
    remove_clients(&v, "closed");
    vector_free(&v);
    if (FD_ISSET(localfd, set))
    {
        unsigned char buffer[2048];
        ssize_t readen;

        readen = read(localfd, buffer, sizeof(buffer));
        if (readen > 0)
        {
            ssize_t idx;
            ipHdr = (struct iphdr*)buffer;
            idx = active_vector_lookup(&this.clients, compare_clients_by_ip, (void*)(long)ipHdr->daddr, sizeof(ipHdr->daddr));
            if (idx >= 0)
            {
                client_t* client;
                size_t len;
                active_vector_get(&this.clients, idx, (void**)&client, &len);
                group = new_msg_group(buffer, readen);
                if (group)
                {
                    ssize_t written = send_msg_group(client->fd, group);
                    msg_group_free(group);
                    SYSLOG(LOG_INFO, "send msg length: %ld", written);
                }
            }
        }
    }
}

void server_loop(int remotefd, int localfd)
{
    fd_set set;
    int max;
    vector_t v;
    vector_functor_t f = {
        vector_dummy_dup,
        NULL
    };

    vector_init(&v, f);
    while (1)
    {
        struct timeval tv = {1, 0};
        active_vector_iterator_t iter;

        FD_ZERO(&set);
        FD_SET(remotefd, &set);
        FD_SET(localfd, &set);
        max = remotefd > localfd ? remotefd : localfd;
        iter = active_vector_begin(&this.clients);
        while (!active_vector_is_end(iter))
        {
            int fd = ((client_t*)iter.data)->fd;
            FD_SET(fd, &set);
            if (fd > max) max = fd;
            iter = active_vector_next(iter);
        }

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max > 0) server_process(max, &set, remotefd, localfd);

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

