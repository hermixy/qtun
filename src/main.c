#include <stdio.h>
#include <stdlib.h>

#include "qvn.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: ./qtun <0|1>\n");
        return 1;
    }

    if (argv[1][0] == '0') init_with_server(6687);
    else init_with_client(inet_addr("162.254.1.197"), 6687);
    system("/home/lwch/start.sh");
    network_loop();
    
    return 0;
}
