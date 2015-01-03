#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
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
    fixed = sizeof(msg_t) + this.max_length;
    for (i = 0; i < g->count - 1; ++i)
    {
        written = write_n(fd, g->elements[i], fixed);
        if (written <= 0) return written;
        ret  += written;
        left -= written;
    }
    written = write_n(fd, g->elements[g->count - 1], left);
    if (written <= 0) return written;
    return ret + written;
}

