#pragma once
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include "status.h"
#include "server/server.h"
#include "network_exceptions.h"

typedef struct client_interface_t
{
    struct sock_server_t *_server;
    int32_t     _socket_descriptor;
    uint32_t    _id;
} client_interface_t;

int client_interface_create(client_interface_t *client, int sock_fd, struct sock_server_t *const server);

void client_interface_close(client_interface_t *client);

void client_interface_close_connection(client_interface_t *client);
