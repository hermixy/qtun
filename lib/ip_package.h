#ifndef _IP_PACKAGE_H_
#define _IP_PACKAGE_H_

#include <stdlib.h>

extern int read_ip_package(int fd, int have_type, void* buffer, size_t* len);

#endif
