#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/icmp.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

static int tun_open(char name[IFNAMSIZ])
{
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1)
    {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) == -1)
    {
        perror("ioctl");
        close(fd);
        return -1;
    }

    strncpy(name, ifr.ifr_name, IFNAMSIZ);
    return fd;
}

static unsigned short checksum(void* buffer, size_t len)
{
    unsigned int sum = 0;
    unsigned short* w = buffer;

    for (; len > 1; len -= sizeof(*w))
    {
        sum += *w++;
    }
    if (len)
    {
        unsigned char tmp[2] = {*(unsigned char*)w, 0x00};
        sum += *(unsigned short*)tmp;
    }
    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

int main()
{
    char name[IFNAMSIZ];
    unsigned char cmd[1024];
    int fd;
    
    memset(name, 0, IFNAMSIZ);
    fd = tun_open(name);
    if (fd == -1) return 1;
    fprintf(stdout, "%s opened\n", name);
    
    sprintf(cmd, "ifconfig %s 10.0.1.1 up", name);
    system(cmd);
    sprintf(cmd, "route add 10.0.1.2 dev %s", name);
    system(cmd);
    
    while (1)
    {
        ssize_t readen = read(fd, cmd, sizeof(cmd));
        if (readen > 0)
        {
            struct iphdr* ipHdr = (struct iphdr*)cmd;
            if (ipHdr->version == 4 && ipHdr->protocol == IPPROTO_ICMP) // 只处理ICMP的IPV4包
            {
                unsigned char ipHdrLen = ipHdr->ihl << 2;
                struct icmphdr* icmpHdr = (struct icmphdr*)(cmd + ipHdrLen); // 跳过IP头和可选头
                if (icmpHdr->type == ICMP_ECHO) // 只处理ECHO包
                {
                    unsigned short ipLen = ntohs(ipHdr->tot_len) - ipHdrLen;

                    __be32 tmp = ipHdr->saddr;
                    ipHdr->saddr = ipHdr->daddr;
                    ipHdr->daddr = tmp;
                    ipHdr->check = 0;
                    ipHdr->check = checksum(ipHdr, ipHdrLen);

                    icmpHdr->type = ICMP_ECHOREPLY;
                    icmpHdr->checksum = 0;
                    icmpHdr->checksum = checksum(icmpHdr, ipLen);
                    
                    write(fd, cmd, ipHdrLen + ipLen);
                }
            }
        }
    }
    return 0;
}
