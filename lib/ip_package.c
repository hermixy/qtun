#include <linux/ip.h>
//#include <linux/ipv6.h>
#include <limits.h>

#include "qvn.h"
#include "network.h"
#include "ip_package.h" 

int read_ip_package(int fd, int have_type, void* buffer, size_t* len)
{
    struct iphdr* hdrv4;
    int rc;
    int type;
    struct iovec iv[2];

    if (have_type) // tun dev
    {
        iv[0].iov_base = (char *)&type;
        iv[0].iov_len = sizeof (type);
        iv[1].iov_base = buffer;
        iv[1].iov_len = USHRT_MAX;
        rc = readv(fd, iv, 2);
    }
    else // network
    {
        int is_tcp;
        if (conf.type == QVN_CONF_TYPE_SERVER)
        {
            server_network_t* network = network_this;
            is_tcp = network->protocol == NETWORK_PROTOCOL_TCP;
        }
        else
        {
            client_network_t* network = network_this;
            is_tcp = network->protocol == NETWORK_PROTOCOL_TCP;
        }
        if (is_tcp) rc = read(fd, buffer, USHRT_MAX);
        else rc = recvfrom(fd, buffer, USHRT_MAX, 0, NULL, NULL);
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
