#include <arpa/inet.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "network.h"

int bind_and_listen(unsigned short port)
{
    int fd, rc;
    int opt = 1;
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);

    this.bind_fd = fd = socket(AF_INET, SOCK_STREAM, 0);
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

int connect_server(char* host, unsigned short port)
{
    int fd, rc;
    struct hostent* he;
    struct sockaddr_in addr = {0};
    int flag = 1;

    this.remote_fd = fd = socket(AF_INET, SOCK_STREAM, 0);
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

    return fd;
}

static void _accept()
{
    int fd = accept(this.bind_fd, NULL, NULL);
    int flag = 1;
    if (fd == -1 || this.client_fd != -1) return;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
    {
        perror("setsockopt");
    }

    this.client_fd = fd;
}

static ssize_t read_pre(int fd, void* buffer, size_t count)
{
    return read(fd, buffer, count);
}

static ssize_t write_n(int fd, void* buffer, size_t count)
{
    char* ptr = buffer;
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

static void process(int max, fd_set* set)
{
    ssize_t len;

    if (FD_ISSET(this.bind_fd, set)) // accept
    {
        _accept();
    }

    if (this.client_fd != -1 && FD_ISSET(this.client_fd, set)) // client
    {
        len = read_pre(this.client_fd, this.buffer, this.buffer_len);
        if (len > 0) write_n(this.remote_fd, this.buffer, len);
    }

    if (FD_ISSET(this.remote_fd, set)) // server
    {
        len = read_pre(this.remote_fd, this.buffer, this.buffer_len);
        if (len > 0) write_n(this.client_fd, this.buffer, len);
    }
}

void loop()
{
    fd_set set;
    int max;

    while (1)
    {
        struct timeval tv = {1, 0};

        FD_ZERO(&set);
        FD_SET(this.bind_fd, &set);
        FD_SET(this.remote_fd, &set);
        if (this.client_fd != -1) FD_SET(this.client_fd, &set);
        if (this.bind_fd > this.remote_fd) max = this.bind_fd;
        if (this.client_fd > max) max = this.client_fd;

        max = select(max + 1, &set, NULL, NULL, &tv);
        if (max > 0) process(max, &set);
    }
}

