#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdlib.h>
#ifndef WIN32
#include <syslog.h>
#endif

#include "library.h"
#include "win.h"

#define LEN2MASK(len) ((1 << (len)) - 1)

#ifdef WIN32
#define SWAP(a, b, type) do { \
    type tmp = a; \
    a = b; \
    b = tmp; \
} while (0)

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#else
#define SWAP(a, b) \
({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void)(&_a == &_b); \
    _a = a; \
    a = b; \
    b = _a; \
})

#define MIN(a, b) \
({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void)(&_a == &_b); \
    a < b ? a : b; \
})

#define MAX(a, b) \
({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void)(&_a == &_b); \
    a > b ? a : b; \
})
#endif

#define SHOW_TIME_DIFF(pfx, a, b) \
do { \
    printf(pfx, (b.tv_sec - a.tv_sec) + (b.tv_usec - a.tv_usec) / 1000000.0); \
} while (0)

#define SYSTEM_NORMAL(cmd) \
do {\
    if (system(cmd) == -1) \
    { \
        perror("system"); \
    } \
} while (0)

#define SYSTEM_EXIT(cmd) \
do {\
    if (system(cmd) == -1) \
    { \
        perror("system"); \
        exit(1); \
    } \
} while (0)

#ifdef WIN32
#define SYSLOG(level, ...)
#else
#define SYSLOG(level, arg...) \
do { \
if (this.log_level >= level) syslog(level, ##arg); \
} while(0)
#endif

#define ROUND_UP(bytes, align) (((bytes) + (align) - 1) & ~((align) - 1))
#define ROUND_DOWN(bytes, align) ((bytes) & ~((align) - 1))

extern unsigned short checksum(void* buffer, size_t len);

extern uint32_t little32(uint32_t n);
extern uint16_t little16(uint16_t n);
extern uint32_t big32(uint32_t n);
extern uint16_t big16(uint16_t n);
extern uint32_t little2host32(uint32_t n);
extern uint16_t little2host16(uint16_t n);

#endif
