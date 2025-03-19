#include <errno.h>
#include <stdlib.h>

#include "status.h"
#include "server/server.h"
#include "server/client.h"
#include "network_exceptions.h"

#define MAX_IPV6_LEN 30

static int sock_server_bind_ipv4(sock_server_t *server, const char *lhost, in_port_t lport);

static int sock_server_bind_ipv6(sock_server_t *server, const char *lhost, in_port_t lport);

static uint32_t sock_server_get_scope_id(const sock_server_t *server, const char *interface_name);

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

    typedef int (*bind_ptr)(sock_server_t *, const char *, in_port_t);

    sa_family_t domain = AF_INET;
    bind_ptr bind_func = &sock_server_bind_ipv4;
    if (use_ipv6)
    {
        domain = AF_INET6;
        bind_func = &sock_server_bind_ipv6;
    }

    server->_socket_descriptor = socket(domain, sock_type, 0);
    if (server->_socket_descriptor == -1)
        return socket_error_init;

    if ((bind_func)(server, lhost, lport) != socket_error_success)
        return socket_error_bind;

    return socket_error_success;
}

void sock_server_close(sock_server_t *server)
{
    sock_server_stop(server);

    if (shutdown(server->_socket_descriptor, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shutdown Server:\t%s", WARNING, strerror(errno));

    if (close(server->_socket_descriptor) != 0)
        fprintf(stderr, "%s Closing Server:\t%s", WARNING, strerror(errno));

    if (server->_address != NULL)
        free(server->_address);
}

static int sock_server_bind_ipv4(sock_server_t *server, const char *lhost, in_port_t lport)
{
    struct sockaddr_in *address_ipv4 = malloc(sizeof(struct sockaddr_in));
    address_ipv4->sin_family = AF_INET;
    address_ipv4->sin_port = htons(lport);
    if (inet_pton(AF_INET, lhost, &address_ipv4->sin_addr) != 1)
    {
        free(address_ipv4);
        return socket_error_invalid_args;
    }

    if (bind(server->_socket_descriptor, (struct sockaddr *)address_ipv4, sizeof(*address_ipv4)) != 0)
    {
        free(address_ipv4);
        return socket_error_bind;
    }

    server->_address = (struct sockaddr *)address_ipv4;
    return socket_error_success;
}

static int sock_server_bind_ipv6(sock_server_t *server, const char *lhost, in_port_t lport)
{
    char ipv6[MAX_IPV6_LEN];
    uint8_t delimeter_index = strcspn(lhost, "%");
    strncpy(ipv6, lhost, delimeter_index);
    const char *scope_id = lhost + delimeter_index;
    struct sockaddr_in6 *address_ipv6 = malloc(sizeof(struct sockaddr_in6));

    address_ipv6->sin6_family = AF_INET6;
    address_ipv6->sin6_port = htons(lport);

    if (inet_pton(AF_INET6, ipv6, &address_ipv6->sin6_addr) != 1)
    {
        free(address_ipv6);
        return socket_error_invalid_args;
    }

    address_ipv6->sin6_scope_id = atoi(scope_id);
    address_ipv6->sin6_scope_id = sock_server_get_scope_id(server, scope_id);

    if (bind(server->_socket_descriptor, (struct sockaddr *)address_ipv6, sizeof(*address_ipv6)) != 0)
    {
        free(address_ipv6);
        return socket_error_bind;
    }

    server->_address = (struct sockaddr *)address_ipv6;
    return socket_error_success;
}

static uint32_t sock_server_get_scope_id(const sock_server_t *server, const char *interface_name)
{
    struct ifreq interface_descriptor;
    interface_descriptor.ifr_addr.sa_family = AF_INET;
    strncpy(interface_descriptor.ifr_name, interface_name, IFNAMSIZ - 1);

    // Get interface general info
    ioctl(server->_socket_descriptor, SIOCGIFADDR, &interface_descriptor);
    return interface_descriptor.ifr_ifru.ifru_ivalue;
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
    int new_client_socket = accept(server->_socket_descriptor, server->_address, &socket_length);
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
