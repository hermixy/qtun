#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include <stdlib.h>

typedef struct
{
    void*  key;
    void*  val;
    size_t key_len;
    size_t val_len;
} pair_t;

#ifdef WIN32
typedef long ssize_t;
typedef long socklen_t;
#endif

#endif
