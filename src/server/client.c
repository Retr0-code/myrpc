#include <errno.h>

#include "status.h"
#include "server/client.h"
#include "server/server.h"
#include "network_exceptions.h"

static uint32_t _clients_amount = 0;

int ClientInterface_create(ClientInterface *client, int sock_fd, Server *const server)
{
    if (server == NULL || client == NULL || sock_fd < 0)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }
    
    Server **const client_server = &client->_server;
    client->_socket_descriptor = sock_fd;
    *client_server = server;
    client->_id = _clients_amount++;
    return socket_error_success;
}

void ClientInterface_close(ClientInterface *client)
{
    if (shutdown(client->_socket_descriptor, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shutdown Client:\t%s\n", ERROR, strerror(errno));
    
    if (close(client->_socket_descriptor) != 0)
        fprintf(stderr, "%s Closing Client:\t%s\n", ERROR, strerror(errno));

    Server_disconnect(client->_server, client->_id);
    --_clients_amount;
}

void ClientInterface_close_connection(ClientInterface *client)
{
    ClientInterface_close(client);
    Server_disconnect(client->_server, client->_id);
    --_clients_amount;
}
