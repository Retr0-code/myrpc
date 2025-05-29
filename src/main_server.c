#include "myrpc/myrpc_server.h"

int main(int argc, char **argv)
{
    rpc_server_t myrpc;

    if (rpc_server_read_config(&myrpc.config, NULL) != rpce_success)
        return -1;

    if (rpc_server_create(&myrpc, &myrpc.config))
       return -1;

    rpc_server_run(&myrpc);
    return 0;
}
