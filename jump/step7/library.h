#ifndef _LIBRARY_H_
#define _LIBRARY_H_

typedef struct
{
    unsigned int msg_ident;
} this_t;

extern this_t this;

typedef struct
{
    int use_gzip;
    int use_aes;
    int use_des;
} library_conf_t;

extern int library_init(library_conf_t conf);

#endif

