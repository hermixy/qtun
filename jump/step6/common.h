#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>

extern unsigned short checksum(void* buffer, size_t len);

extern size_t fd_hash(const void* fd, const size_t len);
extern int fd_compare(const void* fd1, const size_t l1, const void* fd2, const size_t l2);
extern void* fd_dup(const void* fd, const size_t len);
extern int hash2fd(void* fd);

extern size_t ip_hash(const void* ip, const size_t len);
extern int ip_compare(const void* ip1, const size_t l1, const void* ip2, const size_t l2);
extern void* ip_dup(const void* ip, const size_t len);
extern struct in_addr hash2ip(void* ip);

#endif

