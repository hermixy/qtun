#include <stdio.h>

#include "rand.h"

static unsigned short x[] = {
    0x1415, 0x9265,
    0x3589, 0x7932
};

static unsigned short y[] = {
    0x3846, 0x2643,
    0x3832, 0x7950
};

static unsigned int k = 0xBCDCCDCB;

#define lmove32(a, b, c) (a = b, b = (c))

void srand32(int seed)
{
    int i;
    unsigned short a = (seed >> 16), b = seed;
    for (i = 0; i < 2; ++i)
    {
        x[i] = a * x[i] + b * y[i] + k;
        lmove32(a, b, x[i]);
        y[i] = a * x[i] + b * y[i] + k;
        lmove32(a, b, y[i]);
        k = x[i] ^ y[i];
    }
}

void srand64(long long seed)
{
}

static void _rand()
{
    unsigned int a = x[0], b = y[0], c = x[1], d = y[1];
    
    x[0] = a * x[b > 0x7FFF ? 1 : 2] + b * c + d;
    a = x[0] > 0x7FFF ? x[0] : x[1];
    c = d;
    d = y[2] = a * y[b > 0x7FFF ? 1 : 2] + b * c + d;
    
    x[1] = a * x[c > 0x7FFF ? 2 : 3] + b * c + d;
    a = x[1] > 0x7FFF ? x[1] : x[2];
    b = d;
    d = y[3] = a * y[c > 0x7FFF ? 2 : 3] + b * c + d;
    
    x[2] = a * x[d > 0x7FFF ? 3 : 0] + b * c + d;
    a = x[2] > 0x7FFF ? x[2] : x[3];
    b = c;
    c = y[0] = a * y[d > 0x7FFF ? 3 : 0] + b * c + d;
    
    x[3] = a * x[a > 0x7FFF ? 0 : 1] + b * c + d;
    a = x[0], b = y[0], c = x[1], d = y[1];
}

int rand32()
{
    _rand();
    return (x[0] << 16) | y[0];
}

long long rand64()
{
    long long ret;
    _rand();
    ret = x[0];
    ret <<= 48;
    ret |= y[0];
    ret <<= 32;
    ret |= x[1];
    ret <<= 16;
    ret |= y[1];
    return ret;
}
