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

static unsigned short k = 0xBCDC;

#define SEED(a, b, c, d) {(c) = (a) * (c) + (b) * (d) + (k); LMOVE(a, b, (c));}
#define LMOVE(a, b, c) ((a) = (b), (b) = (c))
#define LOW(x)  ((x) & 0xFFFF)
#define HIGH(x) (((x) >> 16) & 0xFFFF)
#define COIN(x) ((x) > 65535)
#define SINGLE(x) ((x) & 1)
#define MUL(x, y, z) {unsigned int n = (x) * (y); (z)[0] = LOW(n); (z)[1] = HIGH(n);}
#define ADD(x, y, z) (z = COIN((x) + (y)), x = LOW((x) + (y)), y = LOW((x) + (z)))

void srand32(int seed)
{
    int i;
    unsigned short a = (seed >> 16), b = seed;
    for (i = 0; i < 2; ++i)
    {
        SEED(a, b, x[i], y[i]);
        SEED(a, b, y[i], x[i]);
        k = x[i] ^ y[i];
    }
}

void srand64(long long seed)
{
    int i;
    unsigned short a = (seed >> 48), b = (seed >> 32), c = (seed >> 16), d = seed;
    for (i = 0; i < 4; ++i)
    {
        if (SINGLE(a + b))
        {
            if (SINGLE(c + d))
            {
                SEED(a, b, x[i], y[i]);
                SEED(a, b, y[i], x[i]);
            }
            else
            {
                SEED(b, a, x[i], y[i]);
                SEED(b, a, y[i], x[i]);
            }
        }
        else if (SINGLE(a + c))
        {
            if (SINGLE(b + d))
            {
                SEED(a, c, x[i], y[i]);
                SEED(a, c, y[i], x[i]);
            }
            else
            {
                SEED(c, a, x[i], y[i]);
                SEED(c, a, y[i], x[i]);
            }
        }
        else if (SINGLE(a + d))
        {
            if (SINGLE(b + c))
            {
                SEED(a, d, x[i], y[i]);
                SEED(a, d, y[i], x[i]);
            }
            else
            {
                SEED(d, a, x[i], y[i]);
                SEED(d, a, y[i], x[i]);
            }
        }
        else if (SINGLE(b + c))
        {
            if (SINGLE(a + d))
            {
                SEED(b, c, x[i], y[i]);
                SEED(b, c, y[i], x[i]);
            }
            else
            {
                SEED(c, b, x[i], y[i]);
                SEED(c, b, y[i], x[i]);
            }
        }
        else if (SINGLE(b + d))
        {
            if (SINGLE(a + c))
            {
                SEED(b, d, x[i], y[i]);
                SEED(b, d, y[i], x[i]);
            }
            else
            {
                SEED(d, b, x[i], y[i]);
                SEED(d, b, y[i], x[i]);
            }
        }
        else
        {
            if (SINGLE(a + b))
            {
                SEED(c, d, x[i], y[i]);
                SEED(c, d, y[i], x[i]);
            }
            else
            {
                SEED(d, c, x[i], y[i]);
                SEED(d, c, y[i], x[i]);
            }
        }
        k = x[i] ^ y[i];
    }
}

static void _rand()
{
    unsigned int a[2], b[2], c[2], d[2], coin[4];
    unsigned int tmp;
    
    MUL(x[0], y[0], a);
    ADD(a[0], k, coin[0]);
    
    MUL(x[1], y[1], b);
    ADD(b[1], k, coin[1]);
    
    MUL(x[2], y[2], c);
    ADD(c[0], k, coin[2]);
    
    MUL(x[3], y[3], d);
    ADD(d[1], k, coin[3]);
    
    tmp = coin[3] + coin[2] + COIN(c[1] + d[0]) + y[2] + x[2] * y[3];
    x[3] = LOW(tmp);
    tmp = coin[1] + coin[0] + COIN(c[0] + d[1]) + x[3] * y[2] + y[3];
    x[2] = LOW(tmp);
    tmp = coin[1] + coin[0] + COIN(a[1] + b[0]) + y[0] + x[0] * y[1];
    x[1] = LOW(tmp);
    tmp = coin[3] + coin[2] + COIN(a[0] + a[1]) + x[1] * y[0] + y[1];
    x[0] = LOW(tmp);
    
    MUL(x[0], y[3], a);
    ADD(a[1], k, coin[0]);
    
    MUL(x[1], y[2], b);
    ADD(b[0], k, coin[1]);
    
    MUL(x[2], y[1], c);
    ADD(c[1], k, coin[2]);
    
    MUL(x[3], y[0], d);
    ADD(d[0], k, coin[3]);
    
    y[0] = LOW(a[0] + coin[3] + d[1]);
    y[1] = LOW(a[0] + coin[2] + coin[1] + c[0]);
    y[2] = LOW(b[1] + coin[3] + c[0]);
    y[3] = LOW(b[1] + coin[2] + coin[1] + a[0]);
}

int rand32()
{
    _rand();
    return (x[2] << 16) | x[1];
}

long long rand64()
{
    long long ret;
    _rand();
    ret = x[0];
    ret <<= 48;
    ret |= x[1];
    ret <<= 32;
    ret |= x[2];
    ret <<= 16;
    ret |= x[3];
    return ret;
}
