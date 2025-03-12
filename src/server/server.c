#include <errno.h>
#include <stdlib.h>

#include "status.h"
#include "server/client.h"
#include "server/server.h"
#include "network_exceptions.h"

#define MAX_IPV6_LEN 30

static int Server_bind_ipv4(Server *server, const char *lhost, in_port_t lport);

static int Server_bind_ipv6(Server *server, const char *lhost, in_port_t lport);

// static int Server_listen_connection(Server *server);

static uint32_t Server_get_scope_id(const Server *server, const char *interface_name);

// static int Server_accept_client(Server *server);

int Server_create(
    Server *server,
    const char *lhost,
    in_port_t lport,
    int use_ipv6,
    uint16_t clients_max_amount)
{
    if (server == NULL)
    {
        errno = EINVAL;
        return socket_error_invalid_args;
    }

    typedef int (*bind_ptr)(Server *, const char *, in_port_t);

    sa_family_t domain = AF_INET;
    bind_ptr bind_func = &Server_bind_ipv4;
    if (use_ipv6)
    {
        domain = AF_INET6;
        bind_func = &Server_bind_ipv6;
    }

    server->_clients_max_amount = clients_max_amount;
    server->_socket_descriptor = socket(domain, SOCK_STREAM, 0);
    if (server->_socket_descriptor == -1)
        return socket_error_init;

    if ((bind_func)(server, lhost, lport) != socket_error_success)
        return socket_error_bind;

    server->_clients = malloc(sizeof(ClientInterface) * server->_clients_max_amount);
    return socket_error_success;
}

void Server_close(Server *server)
{
    Server_stop(server);

    if (shutdown(server->_socket_descriptor, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shutdown Server:\t%s", WARNING, strerror(errno));

    if (close(server->_socket_descriptor) != 0)
        fprintf(stderr, "%s Closing Server:\t%s", WARNING, strerror(errno));

    for (size_t i = 0; i < server->_clients_max_amount; ++i)
        if (server->_clients[i] != NULL)
            ClientInterface_close_connection(server->_clients[i]);

    if (server->_clients != NULL)
        free(server->_clients);

    if (server->_address != NULL)
        free(server->_address);

    free(server);
}

static int Server_bind_ipv4(Server *server, const char *lhost, in_port_t lport)
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

static int Server_bind_ipv6(Server *server, const char *lhost, in_port_t lport)
{
    char ipv6[MAX_IPV6_LEN];
    uint8_t delimeter_index = strcspn(lhost, "%");
    // for (const char *lhost_char_ptr = lhost; *lhost_char_ptr != '%' || delimeter_index != MAX_IPV6_LEN; ++delimeter_index, ++lhost_char_ptr);
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
    address_ipv6->sin6_scope_id = Server_get_scope_id(server, scope_id);

    if (bind(server->_socket_descriptor, (struct sockaddr *)address_ipv6, sizeof(*address_ipv6)) != 0)
    {
        free(address_ipv6);
        return socket_error_bind;
    }

    server->_address = (struct sockaddr *)address_ipv6;
    return socket_error_success;
}

static uint32_t Server_get_scope_id(const Server *server, const char *interface_name)
{
    struct ifreq interface_descriptor;
    interface_descriptor.ifr_addr.sa_family = AF_INET;
    strncpy(interface_descriptor.ifr_name, interface_name, IFNAMSIZ - 1);

    // Get interface general info
    ioctl(server->_socket_descriptor, SIOCGIFADDR, &interface_descriptor);
    return interface_descriptor.ifr_ifru.ifru_ivalue;
}

// Returns thread errors
// int Server_listen(Server *server)
// {
//     int status = thrd_create(&server->_listener, &Server_listen_connection, server);
//     if (status == thrd_success)
//         status = thrd_detach(server->_listener);

//     return status;
// }

// static int Server_listen_connection(Server *server, ClientInterface *client)
int Server_listen_connection(Server *server, ClientInterface *client)
{
    int listen_status = 0;
    server->_stop_listening = 0;

    printf("%s Listening for new connections ...\n", INFO);

    while (!server->_stop_listening)
    {
        listen_status = listen(server->_socket_descriptor, 4);
        if (listen_status < 0)
            return socket_error_listen;

        // if (server->_clients_amount <= server->_clients_max_amount && listen_status == 0)
        server->_stop_listening = !Server_accept_client(server, client);
    }

    return socket_error_success;
}

// static int Server_accept_client(Server *server, ClientInterface *client)
int Server_accept_client(Server *server, ClientInterface *client)
{
    socklen_t socket_length = sizeof(struct sockaddr);
    int new_client_socket = accept(server->_socket_descriptor, server->_address, &socket_length);
    if (new_client_socket < 0)
        return socket_error_listen;

    printf("%s Accepting new client %i\n", SUCCESS, new_client_socket - server->_socket_descriptor);
    // ClientInterface *new_client = malloc(sizeof(ClientInterface));
    ClientInterface_create(client, new_client_socket, server);

    // server->_clients[new_client->_id] = new_client;
    // printf("%s Connected new client %i\n", INFO, new_client->_id);

    // ++server->_clients_amount;

    return socket_error_success;
}

void Server_disconnect(Server *server, uint32_t client_id)
{
    // free(server->_clients[client_id]);
    // server->_clients[client_id] = NULL;
    // memset(&server->_clients[client_id], 0, sizeof(ClientInterface));
    printf("%s Client %i has disconnected\n", INFO, client_id);
    --server->_clients_amount;
}

void Server_stop(Server *server)
{
    printf("%s Stopping server's listener...\n", INFO);
    server->_stop_listening = 1;
}
