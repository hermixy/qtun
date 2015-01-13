#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "library.h"

#include "network.h"

ssize_t read_msg_t(int fd, msg_t** msg, double timeout)
{
    ssize_t rc;
    size_t len;

    *msg = pool_room_alloc(&this.pool, RECV_ROOM_IDX, sizeof(msg_t));
    if (*msg == NULL) return -2;
    rc = read_t(fd, *msg, sizeof(**msg), timeout);
    if (rc <= 0)
    {
        pool_room_free(&this.pool, RECV_ROOM_IDX);
        *msg = NULL;
        return rc;
    }
    len = msg_data_length(*msg);
    *msg = pool_room_realloc(&this.pool, RECV_ROOM_IDX, sizeof(msg_t) + len);
    if (*msg == NULL) return -2;
    rc = read_t(fd, (*msg)->data, len, timeout);
    if (rc <= 0 && len)
    {
        pool_room_free(&this.pool, RECV_ROOM_IDX);
        *msg = NULL;
        return rc;
    }

    if (checksum(*msg, sizeof(msg_t) + len))
    {
        SYSLOG(LOG_ERR, "Invalid msg");
        pool_room_free(&this.pool, RECV_ROOM_IDX);
        *msg = NULL;
        return -2;
    }

    SYSLOG(LOG_INFO, "read msg length: %lu", len);
    return rc + sizeof(msg_t);
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
    void* ptr = buf;
    size_t left = count;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    while (tv.tv_sec || tv.tv_usec)
    {
        ssize_t readen;
        rc = select(fd + 1, &set, NULL, NULL, &tv);
        switch (rc)
        {
        case -1:
            return rc;
        case 0:
            errno = EAGAIN;
            return -1;
        default:
            readen = read(fd, ptr, left);
            if (readen < 0) return readen;
            ptr += readen;
            left -= readen;
            if (left == 0) return count;
            break;
        }
    }
    errno = EAGAIN;
    return -1;
}

inline ssize_t read_pre(int fd, void* buf, size_t count)
{
    return read(fd, buf, count);
}

ssize_t send_msg_group(int fd, msg_group_t* g)
{
    size_t i;
    ssize_t written, ret = 0;
    size_t left, fixed;
    if (g->count == 0) return -1;

    left = msg_data_length(g->elements[0]);
    fixed = this.max_length;
    for (i = 0; i < g->count - 1; ++i)
    {
        written = write_n(fd, g->elements[i], sizeof(msg_t) + fixed);
        if (written <= 0) return written;
        ret  += written;
        left -= fixed;
    }
    written = write_n(fd, g->elements[g->count - 1], sizeof(msg_t) + left);
    if (written <= 0) return written;
    return ret + written;
}

msg_group_t* msg_group_lookup(hash_t* h, size_t ident)
{
    msg_group_t* ret = NULL;
    size_t len = 0;
    if (!hash_get(h, (void*)ident, sizeof(ident), (void**)&ret, &len)) return NULL;
    return ret;
}

size_t msg_ident_hash(const void* data, const size_t len)
{
    return (size_t)data;
}

inline void msg_group_free_hash(void* key, size_t key_len, void* val, size_t val_len)
{
    msg_group_free(val);
}

inline void msg_group_free_hash_val(void* val, size_t len)
{
    msg_group_free(val);
}

int msg_ident_compare(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    return (size_t)d1 == (size_t)d2;
}

int process_clip_msg(int fd, client_t* client, msg_t* msg, size_t* room_id)
{
    size_t i;
    unsigned int ident = ntohl(msg->ident);
    msg_group_t* group = msg_group_lookup(&client->recv_table, ident);
    if (group == NULL)
    {
        group = group_pool_room_alloc(&this.group_pool, sizeof(msg_group_t));
        if (group == NULL)
        {
            SYSLOG(LOG_ERR, "Not enough memory");
            return 0;
        }
        group->count = ceil((double)msg_data_length(msg) / client->max_length);
        group->elements = group_pool_room_alloc(&this.group_pool, sizeof(msg_t*) * group->count);
        memset(group->elements, 0, sizeof(msg_t*) * group->count);
        group->ident = ident;
        group->ttl_start = this.msg_ttl;
        if (!hash_set(&client->recv_table, (void*)(unsigned long)ident, sizeof(ident), group, sizeof(msg_group_t))) return 0;
    }
    if (this.msg_ttl - group->ttl_start > MSG_MAX_TTL) return 0; // expired
    for (i = 0; i < group->count; ++i)
    {
        if (group->elements[i] == NULL) // 收包顺序可能与发包顺序不同
        {
            msg_t* dup = group_pool_room_alloc(&this.group_pool, client->buffer_len);
            if (dup == NULL) break;
            memcpy(dup, msg, client->buffer_len);
            group->elements[i] = dup;
            if (i == group->count - 1)
            {
                void* buffer = NULL;
                unsigned short len = 0;
                if (parse_msg_group(client->max_length, group, &buffer, &len, room_id))
                {
                    ssize_t written = write_n(fd, buffer, len);
                    SYSLOG(LOG_INFO, "write local length: %ld", written);
                    pool_room_free(&this.pool, *room_id);
                }
                else
                    SYSLOG(LOG_WARNING, "Parse message error");
                hash_del(&client->recv_table, (void*)(unsigned long)ident, sizeof(ident));
            }
            break;
        }
    }
    return 1;
}

