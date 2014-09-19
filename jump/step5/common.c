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
