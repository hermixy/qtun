#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "tun.h"
#include "qvn.h"

int tun_open(char name[IFNAMSIZ])
{
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1)
    {
        perror("open");
        return QVN_STATUS_ERR;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) == -1)
    {
        perror("ioctl");
        return QVN_STATUS_ERR;
    }

    strncpy(name, ifr.ifr_name, IFNAMSIZ);
    return fd;
}

