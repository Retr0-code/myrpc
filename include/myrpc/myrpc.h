#pragma once
#include "server/server.h"

#define BUFFER_SIZE 4096

typedef struct
{
    sock_server_t       sock_server;
    client_interface_t  client;
} rpc_server_t;

typedef struct
{
    int socket_type;
    int use_ipv6;
    const char *address;
    uint16_t port;
} rpc_server_config_t;

rpc_server_config_t default_config = {
    SOCK_STREAM,
    0,
    "127.0.0.1",
    135
};

int rpc_server_read_config(rpc_server_config_t *config);

int rpc_server_start(rpc_server_t *server, const rpc_server_config_t *config);

int rpc_server_run(rpc_server_t *server);

int rpc_server_stop(rpc_server_t *server);
