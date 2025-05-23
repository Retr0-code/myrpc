#pragma once

// STD headers
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

// Net linux headers
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

// Own headers
#include "status.h"
#include "sockutils.h"
#include "server/client.h"
#include "network_exceptions.h"

/*
    sock_server_t struct provided to ditribute threads to remote clients.
    Only one instanse of this class is avalible for using. If you
    try to create one more you would get exception
    "server_instance_error" with address of object provided.
*/
typedef struct sock_server_t
{
    sockaddr_u      _address;              // Address descriptor
    atomic_int      _stop_listening;        // State variable for listening thread
    int             _socket_descriptor;     // Descriptor of server socket
    int             _use_ipv6;
    uint16_t        _clients_amount;        // Stores active clients amount
} sock_server_t;

/*  Creates Server instanse with parameters:

    Parameters:
     * const char* lhost - local IPv4 or IPv6
     * in_port_t lport - port to run service on
     * bool use_ipv6 - specifies protocol version IPv6 (by default is false)

    Exceptions:
     * server_instance_error
     * socket_init_error
     * socket_bind_error
*/
int sock_server_create(
    sock_server_t *server,
    const char* lhost,
    in_port_t lport,
    int use_ipv6,
    int sock_type
);

void sock_server_close(sock_server_t *server);

void sock_server_stop(sock_server_t *server);

int sock_server_listen_connection(sock_server_t *server, struct client_interface_t *client);

int sock_server_accept_client(sock_server_t *server, struct client_interface_t *client);

void sock_server_disconnect(sock_server_t *server, uint32_t client_id);
