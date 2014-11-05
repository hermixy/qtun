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

extern unsigned short checksum(void* buffer, size_t len);

extern size_t fd_hash(const void* fd, const size_t len);
extern int fd_compare(const void* fd1, const size_t l1, const void* fd2, const size_t l2);
extern void* fd_dup(const void* fd, const size_t len);
extern int hash2fd(void* fd);

extern size_t ip_hash(const void* ip, const size_t len);
extern int ip_compare(const void* ip1, const size_t l1, const void* ip2, const size_t l2);
extern void* ip_dup(const void* ip, const size_t len);
extern struct in_addr hash2ip(void* ip);

extern uint32_t little32(uint32_t n);
extern uint16_t little16(uint16_t n);
extern uint32_t big32(uint32_t n);
extern uint16_t big16(uint16_t n);
extern void cpy_net32(uint32_t src, uint32_t* dst);
extern void cpy_net16(uint16_t src, uint16_t* dst);
extern uint32_t little2host32(uint32_t n);
extern uint16_t little2host16(uint16_t n);

#endif
