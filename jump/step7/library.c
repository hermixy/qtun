#include <string.h>

#include "msg.h"
#include "library.h"

this_t this;

int library_init(library_conf_t conf)
{
    init_msg_process_handler();

    if (conf.use_aes)
    {
        if (!append_msg_process_handler(MSG_PROCESS_ENCRYPT_HANDLER, MSG_ENCRYPT_AES_ID, aes_encrypt, aes_decrypt))
            return 0;
        memcpy(this.aes_key, conf.aes_key, sizeof(this.aes_key));
        memcpy(this.aes_iv, conf.aes_iv, sizeof(this.aes_iv));
    }

    if (conf.use_gzip)
        if (!append_msg_process_handler(MSG_PROCESS_COMPRESS_HANDLER, MSG_COMPRESS_GZIP_ID, gzip_compress, gzip_decompress))
            return 0;

    return 1;
}
