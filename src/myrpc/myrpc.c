#include "myrpc/myrpc.h"
#include "network_exceptions.h"

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "myrpc.h"

#ifndef RPC_DEFAULT_CONFIG
#define RPC_DEFAULT_CONFIG
rpc_server_config_t default_config = {
    SOCK_STREAM,
    0,
    "127.0.0.1",
    135
};
#endif

static rpc_server_t *rpc_server_static = NULL;

static struct option arguments[] = {
    {"port",     required_argument, 0, 'p'},
    {"host",     required_argument, 0, 'h'},
    {"socktype", required_argument, 0, 's'},
    {"use_ipv6", required_argument, 0, '6'},
};
static const int required_argc = sizeof(arguments) / sizeof(struct option) + 1;

static int str_to_socket_type(const char *str_socktype)
{
    struct pair_t
    {
        const char *str;
        int value;
    } conversion_array[] = {
        {"SOCK_STREAM",     SOCK_STREAM},
        {"SOCK_DGRAM",      SOCK_DGRAM},
        {"SOCK_RAW",        SOCK_RAW},
        {"SOCK_RDM",        SOCK_RDM},
        {"SOCK_SEQPACKET",  SOCK_SEQPACKET},
        {"SOCK_DCCP",       SOCK_DCCP},
        {"SOCK_PACKET",     SOCK_PACKET}
    };

    for (size_t i = 0; i < sizeof(conversion_array) / sizeof(struct pair_t); ++i)
    {
        if (!strcmp(str_socktype, conversion_array[i].str))
            return conversion_array[i].value;
    }
    return -1;
}

static void signal_handler_stop(int signal)
{
    rpc_server_stop(rpc_server_static);
    exit(0);
}

static int rpc_parse_config(rpc_server_config_t *config, char **argv, int argc)
{
    int arg_index = 0;
    int option = 0;
    int args_amount = required_argc - 1;
    int flag = 0;

    while ((option = getopt_long(argc, argv, "p:h:s:6:", arguments, &arg_index)) != -1)
    {
        if (flag & (1 << arg_index))
            continue;

        switch (option)
        {
            case 'p':
            {
                uint16_t port = atoi(optarg);
                config->port = port == 0 ? config->port : port;
                break;
            }

            case 'h':
            {
                config->address = optarg;
                break;
            }
            case 's':
            {
                int sock_type = str_to_socket_type(optarg);
                config->socket_type = sock_type == -1 ? config->socket_type : sock_type;
                break;
            }

            case '6':
            {
                config->use_ipv6 = atoi(optarg);
                break;
            }

            default:
                break;
        }
        flag |= (1 << arg_index);
    }
}

int rpc_server_read_config(rpc_server_config_t *config, const char *filepath)
{
    if (config == NULL)
    {
        errno = EINVAL;
        return rpc_invalid_args;
    }
    memcpy(config, &default_config, sizeof(rpc_server_config_t));

    if (filepath == NULL)
        return rpc_success;

    FILE *config_file = fopen(filepath, "r");
    if (config_file == NULL)
        return rpc_config_error;

    char *argv[required_argc];
    int argc = 0;
    memset(argv, 0, required_argc * sizeof(char *));
    *argv = "/";

    char *line = NULL;
    size_t length = 0;
    char *arg_line = NULL;
    while (getline(&line, &length, config_file) != -1)
    {
        ++argc;
        size_t line_length = strlen(line);
        argv[argc] = malloc(line_length + 2);
        argv[argc][0] = '-';
        argv[argc][1] = '-';
        memcpy(argv[argc] + 2, line, line_length);
    }
    free(line);
    fclose(config_file);

    rpc_parse_config(config, argv, argc);

    for (; argc != 1; --argc)
        free(argv[argc]);

    return rpc_success;
}

int rpc_server_create(rpc_server_t *server, const rpc_server_config_t *config)
{
    if (config == NULL || server == NULL || config->address == NULL || config->port == 0)
    {
        errno = EINVAL;
        return rpc_invalid_args;
    }
    rpc_server_static = server;

    int status = rpc_success;
    if ((status = sock_server_create(
        &server->sock_server,
        config->address,
        config->port,
        config->use_ipv6,
        config->socket_type
    )) != socket_error_success)
    {
        fprintf(stderr, "%s Failed to create server\n", ERROR);
        return status;
    }

    signal(SIGINT,  &signal_handler_stop);
    signal(SIGKILL, &signal_handler_stop);
    signal(SIGTERM, &signal_handler_stop);

    return rpc_success;
}

int rpc_server_run(rpc_server_t *server)
{
    char msg[BUFFER_SIZE];
    while (!sock_server_listen_connection(&server->sock_server, &server->client))
    {
        memset(msg, 0, BUFFER_SIZE);
        if (read(server->client._socket_descriptor, msg, BUFFER_SIZE) != 0)
            printf("Client sent message: %s\n", msg);

        if (strncmp("close\n", msg, 6) == 0)
            client_interface_close_connection(&server->client);
    }
}

void rpc_server_stop(rpc_server_t *server)
{
    server->sock_server._stop_listening = 1;
}

void rpc_server_close(rpc_server_t *server)
{
    client_interface_close(&rpc_server_static->client);
    sock_server_close(&rpc_server_static->sock_server);
}
