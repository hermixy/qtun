#ifndef WIN32
#include <netinet/in.h>
#include <byteswap.h>
#endif

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

uint32_t little32(uint32_t n)
{
    if (!this.little_endian) return bswap_32(n);
    return n;
}

uint16_t little16(uint16_t n)
{
    if (!this.little_endian) return bswap_16(n);
    return n;
}

uint32_t big32(uint32_t n)
{
    if (this.little_endian) return bswap_32(n);
    return n;
}

uint16_t big16(uint16_t n)
{
    if (this.little_endian) return bswap_16(n);
    return n;
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
