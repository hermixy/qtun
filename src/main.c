#include <stdlib.h>

#include "qvn.h"

int main(int argc, char* argv[])
{
    init_with_server(6687);
    network_loop();
    return 0;
}
