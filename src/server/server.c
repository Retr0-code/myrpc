#include <errno.h>
#include <stdlib.h>

#include "status.h"
#include "server/server.h"
#include "server/client.h"
#include "network_exceptions.h"

int sock_server_create(
    sock_server_t *server,
    const char *lhost,
    in_port_t lport,
    int use_ipv6,
    int sock_type)
{
    if (server == NULL)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }

    server->_use_ipv6 = (use_ipv6 != 0);
    sa_family_t domain = server->_use_ipv6 ? AF_INET6 : AF_INET;

    server->_socket_descriptor = socket(domain, sock_type, 0);
    if (server->_socket_descriptor == -1)
        return socket_error_init;

    if (socket_bind(&server->_address,
                server->_use_ipv6,
                server->_socket_descriptor,
                lhost, lport
    ) != socket_error_success)
    {
        sock_server_close(server);
        return socket_error_bind;
    }
    
    return socket_error_success;
}

void sock_server_close(sock_server_t *server)
{
    sock_server_stop(server);

    if (shutdown(server->_socket_descriptor, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shutdown Server:\t%s", WARNING, strerror(errno));

    if (close(server->_socket_descriptor) != 0)
        fprintf(stderr, "%s Closing Server:\t%s", WARNING, strerror(errno));
}

int sock_server_listen_connection(sock_server_t *server, client_interface_t *client)
{
    int listen_status = 0;
    server->_stop_listening = 0;

    printf("%s Listening for new connections ...\n", INFO);

    while (!server->_stop_listening)
    {
        listen_status = listen(server->_socket_descriptor, 4);
        if (listen_status < 0)
            return socket_error_listen;

        server->_stop_listening = !sock_server_accept_client(server, client);
    }

    return socket_error_success;
}

int sock_server_accept_client(sock_server_t *server, client_interface_t *client)
{
    socklen_t socket_length = sizeof(struct sockaddr);
    int new_client_socket = accept(server->_socket_descriptor, (struct sockaddr*)&server->_address, &socket_length);
    if (new_client_socket < 0)
        return socket_error_listen;

    printf("%s Accepting new client %i\n", SUCCESS, new_client_socket - server->_socket_descriptor);
    client_interface_create(client, new_client_socket, server);

    return socket_error_success;
}

void sock_server_disconnect(sock_server_t *server, uint32_t client_id)
{
    printf("%s Client %i has disconnected\n", INFO, client_id);
    --server->_clients_amount;
}

void sock_server_stop(sock_server_t *server)
{
    printf("%s Stopping server's listener...\n", INFO);
    server->_stop_listening = 1;
}
