#pragma once
#include "server/server.h"

typedef struct
{
    const rpc_access_list_t *al;
    client_interface_t      client;
} client_args_t;

typedef struct thread_node_t
{
    struct thread_node_t *next;
    client_args_t client_args;
    thrd_t thread;
} thread_node_t;

typedef struct
{
    sock_server_t        sock_server;
    rpc_server_config_t  config;
} rpc_server_t;

typedef struct
{
    struct rpc_access_list_t *next;
    uid_t uid;
} rpc_access_list_t;

typedef struct
{
    rpc_access_list_t access_list;
    const char *address;
    int socket_type;
    int use_ipv6;
    uint16_t port;
} rpc_server_config_t;

typedef struct
{
    char     *username;
    uint32_t serialization_type;
    uint32_t username_length;
} rpc_init_connection_t;

typedef char *rpc_command_t;

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
