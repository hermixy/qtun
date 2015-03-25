#ifndef _PROTO_H_
#define _PROTO_H_

#pragma pack(1)
struct iphdr
{
    unsigned char ihl : 4;
    unsigned char version : 4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

struct udphdr
{
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
};

#if !defined(unix) || (!defined(HAVE_LINUX_TCP_H) && !defined(HAVE_NETINET_TCP_H))
struct tcphdr
{
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t res1 : 4;
    uint8_t doff : 4;
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t rst : 1;
    uint8_t psh : 1;
    uint8_t ack : 1;
    uint8_t urg : 1;
    uint8_t res2 : 2;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};
#endif
#pragma pack(1)

#endif

