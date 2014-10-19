#ifndef _MSG_H_
#define _MSG_H_

#include "link.h"

#define SYS_MSG_AUTH_REQ  1
#define SYS_MSG_AUTH_ACK  2
#define SYS_MSG_AUTH_NAK  3
#define SYS_MSG_AUTH_REP  4
#define SYS_DHCP_REQ      5
#define SYS_DHCP_REP      6
#define SYS_PING_REQ      7
#define SYS_PING_REP      8
#define SYS_REPEAT        9
#define SYS_EXPIRED      10

#define MSG_PROCESS_COMPRESS_HANDLER 1
#define MSG_PROCESS_ENCRYPT_HANDLER  2

#define MSG_COMPRESS_GZIP_ID         1

#define MSG_ENCRYPT_AES_ID           1
#define MSG_ENCRYPT_DES_ID           2

typedef struct
{
    unsigned char  syscontrol : 1; // 是否是系统消息
    unsigned char  compress   : 4; // 压缩算法
    unsigned char  encrypt    : 3; // 加密算法
    unsigned int   ident;          // 序号
    unsigned int   sec;            // 发包时间
    unsigned int   usec   : 20;    // 精确到微秒，little-endian
    unsigned short len    : 12;    // 长度 = len * 16 + pfx
    unsigned char  pfx    : 4;     // pfx
    unsigned char  unused : 4;
    unsigned short checksum;       // 校验和
    unsigned char  data[];         // 数据
} msg_t;

typedef struct
{
    unsigned char op;
    unsigned char data[];
} sys_msg_t;

typedef struct
{
    unsigned int  type;
    unsigned char id;
    int (*do_handler)(const void*, const unsigned int, void**, unsigned int*);
    int (*undo_handler)(const void*, const unsigned int, void**, unsigned int*);
} msg_process_handler_t;

extern link_t msg_process_handlers;

extern int gzip_compress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);
extern int gzip_decompress(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);
extern int aes_encrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);
extern int aes_decrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);
extern int des_encrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);
extern int des_decrypt(const void* src, const unsigned int src_len, void** dst, unsigned int* dst_len);

extern void init_msg_process_handler();
extern int append_msg_process_handler(
    int type,
    int id,
    int (*do_handler)(const void*, const unsigned int, void**, unsigned int*),
    int (*undo_handler)(const void*, const unsigned int, void**, unsigned int*)
);
extern size_t msg_data_length(const msg_t* msg);
extern msg_t* new_sys_msg(const void* data, const unsigned short len);
extern msg_t* new_msg(const void* data, const unsigned short len);
extern int parse_msg(const msg_t* input, int* sys, void** output, unsigned short* output_len);

#endif
