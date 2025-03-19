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

typedef struct
{

} rpc_init_connection_t;

typedef enum
{
    rpc_success,
    rpc_invalid_args,
    rpc_config_error,
} rpc_error_e;

int rpc_server_read_config(rpc_server_config_t *config, const char *filepath);

int rpc_server_create(rpc_server_t *server, const rpc_server_config_t *config);

int rpc_server_run(rpc_server_t *server);

void rpc_server_stop(rpc_server_t *server);

void rpc_server_close(rpc_server_t *server);
