#include "myrpc/myrpc_server.h"

int main(int argc, char **argv)
{
    rpc_server_t myrpc;

    if (rpc_server_read_config(&myrpc.config, NULL) != rpce_success)
    {
        perror("Error while reading config");
        return -1;
    }

    if (rpc_server_create(&myrpc, &myrpc.config))
    {
        perror("Error while setting up the server");
        return -1;
    }

    rpc_server_run(&myrpc);
    return 0;
}
