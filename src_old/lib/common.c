#include <sys/time.h>
#include <stdlib.h>

#include "common.h"

double microtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

long long microtime64()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
