#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client/client.h"
#include "network_exceptions.h"

int sock_client_create(
    sock_client_t *client,
    const char *rhost,
    const char *rport,
    int use_ipv6,
    int sock_type)
{
    if (client == NULL || rhost == NULL || rport == 0)
    {
        errno = EINVAL;
        return -1;
    }

    client->_use_ipv6 = (use_ipv6 != 0);
    sa_family_t af = use_ipv6 ? AF_INET6 : AF_INET;
    if ((client->_socket_descriptor = socket(af, sock_type, 0)) == -1)
        return socket_error_init;

    if (socket_resolve_addr(&client->_address,
        &client->_addr_len,
        sock_type,
        client->_use_ipv6,
        rhost, rport) != socket_error_success)
    {
        socket_shutdown_close(client->_socket_descriptor);
        return socket_error_bind;
    }

    return socket_error_success;
}

int sock_client_connect(sock_client_t *client)
{
    if (client == NULL)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }

    char rhost[INET6_ADDRSTRLEN];
    uint16_t rport = client->_use_ipv6 ? client->_address.addr_v6.sin6_port : client->_address.addr_v4.sin_port;

    socket_get_address(rhost, &client->_address, client->_use_ipv6);
    if (connect(client->_socket_descriptor, (struct sockaddr*)&client->_address, client->_addr_len) == -1)
    {
        fprintf(stderr, "%s Client could not connect to server %s:%u:\t%s\n", ERROR, rhost, rport, strerror(errno));
        return socket_error_bind;
    }

    return socket_error_success;
}

void sock_client_stop(sock_client_t *client)
{
    socket_shutdown_close(client->_socket_descriptor);
}
