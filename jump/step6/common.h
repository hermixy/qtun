#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>

extern unsigned short checksum(void* buffer, size_t len);
extern size_t fd_hash(const void* fd, const size_t len);
extern int fd_compare(const void* fd1, const size_t l1, const void* fd2, const size_t l2);
extern void* fd_dup(const void* fd, const size_t len);
extern int hash2fd(void* fd);

#endif

