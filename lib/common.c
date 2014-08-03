#include <sys/time.h>
#include <stdlib.h>

#include "common.h"

double microtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}
