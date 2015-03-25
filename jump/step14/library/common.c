#ifndef WIN32
#include <netinet/in.h>
#endif

#ifdef HAVE_BYTESWAP_H
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

char* STR_LEN2MASK(uint8_t n)
{
    static char str[16] = { 0 };
    uint32_t b = (1 << n) - 1;
    unsigned char tmp[4];
    tmp[0] =  b        & 0xFF;
    tmp[1] = (b >> 8)  & 0xFF;
    tmp[2] = (b >> 16) & 0xFF;
    tmp[3] = (b >> 24) & 0xFF;
    sprintf(str, "%d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);
    return str;
}

int is_int(const char* ptr, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
    {
        if (ptr[i] < '0' || ptr[i] > '9') return 0;
    }
    return 1;
}

