#include <netinet/in.h>

#include "common.h"

unsigned short checksum(void* buffer, size_t len)
{
    unsigned int sum = 0;
    unsigned short* w = buffer;

    for (; len > 1; len -= sizeof(*w))
    {
        sum += *w++;
    }
    if (len)
    {
        unsigned char tmp[2] = {*(unsigned char*)w, 0x00};
        sum += *(unsigned short*)tmp;
    }
    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

size_t fd_hash(const void* fd, const size_t len)
{
    return *(int*)&fd;
}

int fd_compare(const void* fd1, const size_t l1, const void* fd2, const size_t l2)
{
    return fd1 == fd2;
}

void* fd_dup(const void* fd, const size_t len)
{
    return (void*)fd;
}

int hash2fd(void* fd)
{
    return (long)fd;
}

size_t ip_hash(const void* ip, const size_t len)
{
    return *(unsigned int*)&ip;
}

int ip_compare(const void* ip1, const size_t l1, const void* ip2, const size_t l2)
{
    return ip1 == ip2;
}

void* ip_dup(const void* ip, const size_t len)
{
    return (void*)ip;
}

struct in_addr hash2ip(void* ip)
{
    struct in_addr addr;
    addr.s_addr = *(unsigned int*)ip;
    return addr;
}