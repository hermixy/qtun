#include "msg.h"
#include "library.h"

this_t this;

int library_init(library_conf_t conf)
{
    init_msg_process_handler();

    if (conf.use_gzip)
        if (!append_msg_process_handler(MSG_PROCESS_COMPRESS_HANDLER, MSG_COMPRESS_GZIP_ID, gzip_compress, gzip_decompress))
            return 0;

    return 1;
}
