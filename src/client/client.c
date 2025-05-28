#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client/client.h"
#include "network_exceptions.h"

int sock_client_create(
    sock_client_t *client,
    const char *rhost,
    uint16_t rport,
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

    typedef int (*fill_sockaddr_ptr)(sockaddr_u *, const char *, in_port_t);
    fill_sockaddr_ptr bind_func = use_ipv6 ? &socket_fill_sockaddr_in6 : &socket_fill_sockaddr_in;

    if ((bind_func)(&client->_address, rhost, rport) != socket_error_success)
        return socket_error_bind;

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

    if (connect(client->_socket_descriptor, (struct sockaddr*)&client->_address,
        client->_use_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) == -1)
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
