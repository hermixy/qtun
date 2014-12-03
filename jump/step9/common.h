#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdlib.h>

#define LEN2MASK(len) ((1 << (len)) - 1)

#define SWAP(a, b) \
({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void)(&_a == &_b); \
    _a = a; \
    a = b; \
    b = _a; \
})

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

extern unsigned short checksum(void* buffer, size_t len);

extern uint32_t little32(uint32_t n);
extern uint16_t little16(uint16_t n);
extern uint32_t big32(uint32_t n);
extern uint16_t big16(uint16_t n);
extern void cpy_net32(uint32_t src, uint32_t* dst);
extern void cpy_net16(uint16_t src, uint16_t* dst);
extern uint32_t little2host32(uint32_t n);
extern uint16_t little2host16(uint16_t n);

#endif
