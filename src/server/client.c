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
    
    client->_server = server;
    client->_socket_descriptor = sock_fd;
    client->_id = _clients_amount++;
    client->_use_ipv6 = server->_use_ipv6;
    socklen_t address_len = client->_use_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    if (getpeername(client->_socket_descriptor, (struct sockaddr*)&client->_address, &address_len) == -1)
        return socket_error_init;

    return socket_error_success;
}

void client_interface_close(client_interface_t *client)
{
    socket_shutdown_close(client->_socket_descriptor);

    sock_server_disconnect(client->_server, client->_id);
    --_clients_amount;
}
