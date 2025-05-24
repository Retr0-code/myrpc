#include "myrpc/myrpc.h"

int main(int argc, char **argv)
{
    rpc_server_t myrpc;
    rpc_server_config_t config;

    if (rpc_server_read_config(&config, NULL) != rpce_success)
    {
        perror("Error while reading config");
        return -1;
    }

    if (rpc_server_create(&myrpc, &config))
    {
        perror("Error while setting up the server");
        return -1;
    }

    rpc_server_run(&myrpc);
    return 0;
}
