#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if defined(unix) && defined(HAVE_LINUX_IF_TUN_H)
#include <linux/if_tun.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../library/common.h"
#include "../library/library.h"

#include "network.h"

ssize_t read_msg_t(client_t* client, msg_t** msg, double timeout)
{
    if (this.use_udp)
    {
        *msg = pool_room_realloc(&this.pool, RECV_ROOM_IDX, this.recv_buffer_len);
        return read_t(client, *msg, this.recv_buffer_len, timeout);
    }
    else
    {
        ssize_t rc;
        size_t len;

        *msg = pool_room_realloc(&this.pool, RECV_ROOM_IDX, sizeof(msg_t));
        if (*msg == NULL) return -2;
        rc = read_t(client, *msg, sizeof(**msg), timeout);
        if (rc <= 0)
        {
            pool_room_free(&this.pool, RECV_ROOM_IDX);
            *msg = NULL;
            return rc;
        }
        len = msg_data_length(*msg);
        *msg = pool_room_realloc(&this.pool, RECV_ROOM_IDX, sizeof(msg_t) + len);
        if (*msg == NULL) return -2;
        rc = read_t(client, (*msg)->data, len, timeout);
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
}

#ifdef WIN32
local_fd_type tun_open(char path[MAX_PATH])
{
    return CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
#else
local_fd_type tun_open(char name[IFNAMSIZ])
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

    this.localfd = fd;
    strncpy(name, ifr.ifr_name, IFNAMSIZ);
    return fd;
}
#endif

ssize_t write_c(client_t* client, const void* buf, size_t count)
{
    if (this.use_udp)
    {
        return sendto(this.remotefd, buf, (int)count, 0, (struct sockaddr*)&client->addr, sizeof(client->addr));
    }
    else
    {
        const char* ptr = buf;
        size_t left = count;
        while (left)
        {
            ssize_t written = write(client->fd, ptr, (unsigned int)left);
            if (written == 0) return 0;
            else if (written == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                return -1;
            }
            ptr  += written;
            left -= written;
        }
        return (ssize_t)count;
    }
}

ssize_t read_t(client_t* client, void* buf, size_t count, double timeout)
{
    fd_set set;
    struct timeval tv = {(long)timeout, (long)(timeout * 1000000) % 1000000};
    int rc;
    void* ptr = buf;
    size_t left = count;
    FD_ZERO(&set);
    FD_SET(client->fd, &set);
    while (tv.tv_sec || tv.tv_usec)
    {
        ssize_t readen;
        rc = select(client->fd + 1, &set, NULL, NULL, &tv);
        switch (rc)
        {
        case -1:
            return rc;
        case 0:
            errno = EAGAIN;
            return -1;
        default:
            if (this.use_udp)
            {
                socklen_t len = sizeof(client->addr);
                return recvfrom(client->fd, buf, (int)count, 0, (struct sockaddr*)&client->addr, &len);
            }
            else
            {
                readen = read(client->fd, ptr, (int)left);
                if (readen < 0) return readen;
                ptr = (char*)ptr + readen;
                left -= readen;
                if (left == 0) return (ssize_t)count;
            }
            break;
        }
    }
    errno = EAGAIN;
    return -1;
}

ssize_t read_pre(fd_type fd, void* buf, size_t count)
{
    return read(fd, buf, (unsigned int)count);
}

ssize_t udp_read(fd_type fd, void* buf, size_t count, struct sockaddr_in* srcaddr, socklen_t* addrlen)
{
    return recvfrom(fd, buf, (unsigned int)count, 0, (struct sockaddr*)srcaddr, addrlen);
}

ssize_t send_msg_group(client_t* client, msg_group_t* g)
{
    size_t i;
    ssize_t written, ret = 0;
    size_t left, fixed;
    if (g->count == 0) return -1;

    left = msg_data_length(g->elements[0]);
    fixed = this.max_length;
    for (i = 0; i < g->count - 1UL; ++i)
    {
        written = write_c(client, g->elements[i], sizeof(msg_t) + fixed);
        if (written <= 0) return written;
        ret  += written;
        left -= fixed;
    }
    written = write_c(client, g->elements[g->count - 1], sizeof(msg_t) + left);
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

void msg_group_free_hash(void* key, size_t key_len, void* val, size_t val_len)
{
    msg_group_free(val);
}

void msg_group_free_hash_val(void* val, size_t len)
{
    msg_group_free(val);
}

int msg_ident_compare(const void* d1, const size_t l1, const void* d2, const size_t l2)
{
    return (size_t)d1 == (size_t)d2;
}

int process_clip_msg(local_fd_type fd, client_t* client, msg_t* msg, size_t* room_id)
{
    size_t i;
    unsigned int ident = ntohl(msg->ident);
    size_t all_len = msg_data_length(msg);
    msg_group_t* group = msg_group_lookup(&client->recv_table, ident);
    if (!msg->zone.clip) return 0;
    if (group == NULL)
    {
        group = group_pool_room_alloc(&this.group_pool, sizeof(msg_group_t));
        if (group == NULL)
        {
            SYSLOG(LOG_ERR, "Not enough memory");
            return 0;
        }
        group->count = (unsigned short)ceil((double)all_len / client->max_length);
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
            size_t this_len = sizeof(msg_t) + (msg->zone.last ? all_len % client->max_length : client->max_length);
            msg_t* dup = group_pool_room_alloc(&this.group_pool, this_len);
            if (dup == NULL) break;
            memcpy(dup, msg, this_len);
            group->elements[i] = dup;
            if (i == group->count - 1)
            {
                void* buffer = NULL;
                unsigned short len = 0;
                if (parse_msg_group(client->max_length, group, &buffer, &len, room_id))
                {
                    ssize_t written;
#ifdef WIN32
                    WriteFile(fd, buffer, len, &written, NULL);
#else
                    written = write(fd, buffer, len);
#endif
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

int check_msg(client_t* client, msg_t* msg)
{
    size_t msg_data_len;
    if (msg->zone.clip)
    {
        if (msg->zone.last) msg_data_len = msg_data_length(msg) % client->max_length;
        else msg_data_len = client->max_length;
    }
    else msg_data_len = msg_data_length(msg);
    if (checksum(msg, sizeof(msg_t) + msg_data_len))
    {
        SYSLOG(LOG_ERR, "Invalid msg");
        return 0;
    }
    return 1;
}

#ifdef WIN32
int local_have_data()
{
    unsigned char ret = 0;
    DWORD readen = 0;
    DeviceIoControl(this.localfd, IOCTL_HAVE_DATA, NULL, 0, &ret, sizeof(ret), &readen, NULL);
    return ret;
}
#endif
