#include <linux/ip.h>
//#include <linux/ipv6.h>
#include <limits.h>

#include "qvn.h"
#include "ip_package.h" 

int read_ip_package(int fd, int have_type, void* buffer, size_t* len)
{
    struct iphdr* hdrv4;
    int rc;
    int type;
    struct iovec iv[2];

    if (have_type)
    {
        iv[0].iov_base = (char *)&type;
        iv[0].iov_len = sizeof (type);
        iv[1].iov_base = buffer;
        iv[1].iov_len = USHRT_MAX;
        rc = readv(fd, iv, 2);
    }
    else
    {
        rc = read(fd, buffer, USHRT_MAX);
    }
    if (rc <= 0) return QVN_STATUS_ERR;
    
    hdrv4 = (struct iphdr*)buffer;
    if (hdrv4->version == 4)
    {
        *len = ntohs(hdrv4->tot_len);
    }
    else // TODO: ipv6
    {
        *len = 0;
    }
    
    return QVN_STATUS_OK;
}
