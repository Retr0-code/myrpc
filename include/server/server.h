#pragma once

// STD headers
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <stdatomic.h>

// Net linux headers
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

// Own headers
#include "status.h"
#include "server/client.h"
#include "network_exceptions.h"

/*
    Server class provided to ditribute threads to remote clients.
    Only one instanse of this class is avalible for using. If you
    try to create one more you would get exception
    "server_instance_error" with address of object provided.
*/
typedef struct Server
{
    uint16_t                _clients_max_amount;    // By default is 1000
    uint16_t                _clients_amount;        // Stores active clients amount
    int                     _socket_descriptor;     // Descriptor of server socket
    thrd_t                  _listener;              // Listener thread
    atomic_int              _stop_listening;        // State variable for listening thread
    struct sockaddr         *_address;              // Address descriptor
    struct ClientInterface  **_clients;             // Pointers to clients
} Server;

/*  Creates Server instanse with parameters:

    Parameters:
     * const char* lhost - local IPv4 or IPv6
     * in_port_t lport - port to run service on
     * bool use_ipv6 - specifies protocol version IPv6 (by default is false)
     * uint16_t clients_max_amount - specifies maximum of clients connected to the server

    Exceptions:
     * server_instance_error
     * socket_init_error
     * socket_bind_error
*/
int Server_create(
    Server *server,
    const char* lhost,
    in_port_t lport,
    int use_ipv6,
    uint16_t clients_max_amount
);
    
void Server_close(Server *server);

void Server_stop(Server *server);

/*  Runs detached thread that accepts new clientreturns its id

    Returns thread's errors

    Exceptions:
    * all exceptions of "thrd_create( Function&& f, Args&&... args )" constructor
*/
// int Server_listen(Server *server);

int Server_listen_connection(Server *server, struct ClientInterface *client);

int Server_accept_client(Server *server, struct ClientInterface *client);

void Server_disconnect(Server *server, uint32_t client_id);
