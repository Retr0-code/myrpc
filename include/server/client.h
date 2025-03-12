#pragma once
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include "status.h"
#include "server/server.h"
#include "network_exceptions.h"

typedef struct ClientInterface
{
    uint32_t                _id;
    int32_t                 _socket_descriptor;
    struct Server *const    _server;
} ClientInterface;

int ClientInterface_create(ClientInterface *client, int sock_fd, struct Server *const server);

void ClientInterface_close(ClientInterface *client);

void ClientInterface_close_connection(ClientInterface *client);
