#include <errno.h>

#include "status.h"
#include "server/client.h"
#include "server/server.h"
#include "network_exceptions.h"

static uint32_t _clients_amount = 0;

int client_interface_create(client_interface_t *client, int sock_fd, sock_server_t *const server)
{
    if (server == NULL || client == NULL || sock_fd < 0)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }
    
    // sock_server_t **const client_server = &client->_server;
    client->_server = server;
    client->_socket_descriptor = sock_fd;
    // *client_server = server;
    client->_id = _clients_amount++;
    return socket_error_success;
}

void client_interface_close(client_interface_t *client)
{
    if (shutdown(client->_socket_descriptor, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shutdown Client:\t%s\n", ERROR, strerror(errno));
    
    if (close(client->_socket_descriptor) != 0)
        fprintf(stderr, "%s Closing Client:\t%s\n", ERROR, strerror(errno));

    sock_server_disconnect(client->_server, client->_id);
    --_clients_amount;
}

void client_interface_close_connection(client_interface_t *client)
{
    client_interface_close(client);
    sock_server_disconnect(client->_server, client->_id);
    --_clients_amount;
}
