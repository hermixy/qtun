#include <netinet/in.h>
#include <byteswap.h>

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

uint32_t little32(uint32_t n)
{
#if __BYTE_ORDER == __BIG_ENDIAN
    return bswap_32(n);
#else
    return n;
#endif
}

uint16_t little16(uint16_t n)
{
#if __BYTE_ORDER == __BIG_ENDIAN
    return bswap_16(n);
#else
    return n;
#endif
}

uint32_t big32(uint32_t n)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return bswap_32(n);
#else
    return n;
#endif
}

uint16_t big16(uint16_t n)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return bswap_16(n);
#else
    return n;
#endif
}

void cpy_net32(uint32_t src, uint32_t* dst)
{
    uint8_t* d = (uint8_t*)dst;
#if __BYTE_ORDER == __BIG_ENDIAN
    d[3] = (src      ) & 0xFF;
    d[2] = (src >> 8 ) & 0xFF;
    d[1] = (src >> 16) & 0xFF;
    d[0] = (src >> 24) & 0xFF;
#else
    d[0] = (src      ) & 0xFF;
    d[1] = (src >> 8 ) & 0xFF;
    d[2] = (src >> 16) & 0xFF;
    d[3] = (src >> 24) & 0xFF;
#endif
}

void cpy_net16(uint16_t src, uint16_t* dst)
{
    uint8_t* d = (uint8_t*)dst;
#if __BYTE_ORDER == __BIG_ENDIAN
    d[1] = (src      ) & 0xFF;
    d[0] = (src >> 8 ) & 0xFF;
#else
    d[0] = (src      ) & 0xFF;
    d[1] = (src >> 8 ) & 0xFF;
#endif
}

uint32_t little2host32(uint32_t n)
{
    return little32(n);
}

uint16_t little2host16(uint16_t n)
{
    return little16(n);
}

uint32_t big2host32(uint32_t n)
{
    return big32(n);
}

uint16_t big2host16(uint16_t n)
{
    return big16(n);
}
