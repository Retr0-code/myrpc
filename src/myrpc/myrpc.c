#include "myrpc/myrpc.h"
#include "network_exceptions.h"

#include <errno.h>
#include <signal.h>

static int signal_handler_stop(int signal)
{

}

int rpc_server_start(rpc_server_t *server, const rpc_server_config_t *config)
{
    int status = socket_error_success;
    if (config == NULL || server == NULL || config->address == NULL || config->port == 0)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }

    if ((status = sock_server_create(server, config->address, config->port, config->use_ipv6)) != socket_error_success)
    {
        fprintf(stderr, "%s Failed to create server\n", ERROR);
        return status;
    }

    signal(SIGINT,  &signal_handler_stop);
    signal(SIGKILL, &signal_handler_stop);
    signal(SIGTERM, &signal_handler_stop);

    return socket_error_success;
}

int rpc_server_run(rpc_server_t *server)
{
    char msg[BUFFER_SIZE];
    while (!sock_server_listen_connection(&server, &server->client))
    {
        memset(msg, 0, BUFFER_SIZE);
        if (read(server->client._socket_descriptor, msg, BUFFER_SIZE) != 0)
            printf("Client sent message: %s\n", msg);

        if (strncmp("close\n", msg, 6) == 0)
            client_interface_close_connection(&server->client);
    }
}
