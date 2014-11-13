#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "network.h"

ssize_t read_msg(int fd, msg_t** msg)
{
    ssize_t rc;
    size_t len;

    *msg = malloc(sizeof(msg_t));
    if (*msg == NULL) return -2;
    rc = read_t(fd, *msg, sizeof(**msg), 3);
    if (rc <= 0)
    {
        free(*msg);
        *msg = NULL;
        return rc;
    }
    len = msg_data_length(*msg);
    *msg = realloc(*msg, sizeof(msg_t) + len);
    if (*msg == NULL) return -2;
    rc = read_t(fd, (*msg)->data, len, 3);
    if (rc <= 0 && len)
    {
        free(*msg);
        *msg = NULL;
        return rc;
    }

    if (checksum(*msg, sizeof(msg_t) + len))
    {
        fprintf(stderr, "Invalid msg\n");
        free(*msg);
        *msg = NULL;
        return -2;
    }

    printf("read msg length: %lu\n", len);
    return rc + sizeof(msg_t);
}

ssize_t read_msg_t(int fd, msg_t** msg, double timeout)
{
    ssize_t rc;
    size_t len;

    *msg = malloc(sizeof(msg_t));
    if (*msg == NULL) return -2;
    rc = read_t(fd, *msg, sizeof(**msg), timeout);
    if (rc <= 0)
    {
        free(*msg);
        *msg = NULL;
        return rc;
    }
    len = msg_data_length(*msg);
    *msg = realloc(*msg, sizeof(msg_t) + len);
    if (*msg == NULL) return -2;
    rc = read_t(fd, (*msg)->data, len, timeout);
    if (rc <= 0 && len)
    {
        free(*msg);
        *msg = NULL;
        return rc;
    }

    if (checksum(*msg, sizeof(msg_t) + len))
    {
        fprintf(stderr, "Invalid msg\n");
        free(*msg);
        *msg = NULL;
        return -2;
    }

    printf("read msg length: %lu\n", len);
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
    void* ptr;
    size_t left = count;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    while (tv.tv_sec && tv.tv_usec)
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
            if (readen <= 0) return readen;
            break;
        }
    }
}
