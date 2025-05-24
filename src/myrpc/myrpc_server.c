#include "list.h"
#include "myrpc/myrpc_proto.h"
#include "network_exceptions.h"
#include "myrpc/myrpc_server.h"

#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <threads.h>

#ifndef RPC_DEFAULT_CONFIG
#define RPC_DEFAULT_CONFIG
rpc_server_config_t default_config = {
    SOCK_STREAM,
    0,
    "127.0.0.1",
    135};
#endif

static rpc_server_t *rpc_server_static = NULL;

static struct option arguments[] = {
    {"port", required_argument, 0, 'p'},
    {"host", required_argument, 0, 'h'},
    {"socktype", required_argument, 0, 's'},
    {"use_ipv6", required_argument, 0, '6'},
    {"access_list", required_argument, 0, 'a'}};
static const int required_argc = sizeof(arguments) / sizeof(struct option) + 1;

static int str_to_socket_type(const char *str_socktype)
{
    struct pair_t
    {
        const char *str;
        int value;
    } conversion_array[] = {
        {"SOCK_STREAM", SOCK_STREAM},
        {"SOCK_DGRAM", SOCK_DGRAM},
        // {"SOCK_RAW",        SOCK_RAW},
        // {"SOCK_RDM",        SOCK_RDM},
        // {"SOCK_SEQPACKET",  SOCK_SEQPACKET},
        // {"SOCK_DCCP",       SOCK_DCCP},
        // {"SOCK_PACKET",     SOCK_PACKET}
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

static void thread_node_destructor(void *ptr)
{
    thread_node_t *node = ptr;
    thrd_join(node->thread, NULL);
}

static int rpc_read_access_list(const char *al_path, rpc_access_list_t *al)
{
    if (al_path == NULL || al == NULL)
    {
        errno = EINVAL;
        return rpce_invalid_args;
    }

    FILE *al_file = fopen(al_path, "r");
    if (al_file == NULL)
        return rpce_resource;

    char *username = NULL;
    size_t capacity = 0;
    while (getline(&username, &capacity, al_file) != -1)
    {
        passwd *user = getpwnam(username);
        if (user == NULL)
            continue;

        al->uid = user->pw_uid;
        al->next = malloc(sizeof(rpc_access_list_t));
        al = al->next;
    }
    al->next = NULL;

    free(username);
    fclose(al_file);
    return rpce_success;
}

// TODO add logging of config default/errors
static int rpc_parse_config(rpc_server_config_t *config, char **argv, int argc)
{
    int arg_index = 0;
    int option = 0;
    int args_amount = required_argc - 1;
    int flag = 0;

    while ((option = getopt_long(argc, argv, "p:h:s:6:a:", arguments, &arg_index)) != -1)
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
            config->use_ipv6 = (atoi(optarg) > 0);
            break;
        }

        case 'a':
        {
            if (rpc_read_access_list(optarg, &config->access_list) != rpce_success)
                //TODO set to default
                return rpce_user;

            break;
        }

        default:
            break;
        }
        flag |= (1 << arg_index);
    }

    return rpce_success;
}

int rpc_server_read_config(rpc_server_config_t *config, const char *filepath)
{
    if (config == NULL)
    {
        errno = EINVAL;
        return rpce_invalid_args;
    }
    memcpy(config, &default_config, sizeof(rpc_server_config_t));

    if (filepath == NULL)
        return rpce_success;

    FILE *config_file = fopen(filepath, "r");
    if (config_file == NULL)
        return rpce_config_error;

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

    return rpce_success;
}

int rpc_server_create(rpc_server_t *server, const rpc_server_config_t *config)
{
    if (config == NULL || server == NULL || config->address == NULL || config->port == 0)
    {
        errno = EINVAL;
        return rpce_invalid_args;
    }
    rpc_server_static = server;

    int status = rpce_success;
    if ((status = sock_server_create(
             &server->sock_server,
             config->address,
             config->port,
             config->use_ipv6,
             config->socket_type)) != socket_error_success)
    {
        fprintf(stderr, "%s Failed to create server\n", ERROR);
        return status;
    }

    signal(SIGINT,  &signal_handler_stop);
    signal(SIGTERM, &signal_handler_stop);

    return rpce_success;
}

static int rpc_user_in_list(rpc_access_list_t *al, uid_t uid)
{
    for (; al != NULL; al = al->next)
        if (al->uid == uid)
            return rpce_success;

    return rpce_user;
}

static int rpc_try_exec(uid_t uid, char *command)
{

}

static int rpc_client_handle(client_args_t *args)
{
    // Get username
    char *username = NULL;
    if (rpc_receive_message(args->client._socket_descriptor, &username) != me_success)
    {
        // Log & exit
        return rpce_user;
    }

    passwd *record = getpwnam(username);
    if (record == NULL)
    {
        // Log & exit
        rpc_send_close(args->client._socket_descriptor, rpce_user);
        return rpce_user;
    }

    // Check access list
    if (rpc_user_in_list(args->al, record->pw_uid) != rpce_success)
    {
        // Log & exit
        rpc_send_close(args->client._socket_descriptor, rpce_user);
        return rpce_user;
    }

    // Get command
    char *command = NULL;
    if (rpc_receive_command(args->client._socket_descriptor, &command) != me_success)
    {
        // Log & exit
        return rpce_command;
    }

    // Try execute command & Write output to tmpfile
    if (rpc_try_exec(record->pw_uid, command) != rpce_success)
    {
        // Log & exit
        rpc_send_close(args->client._socket_descriptor, rpce_command);
        return rpce_command;
    }

    // Send output
    return rpce_success;
}

int rpc_server_run(rpc_server_t *server)
{
    if (server == NULL)
    {
        errno = EINVAL;
        return rpce_invalid_args;
    }

    thread_node_t *threads = malloc(sizeof(thread_node_t));
    if (threads == NULL)
        return rpce_resource;

    int status = rpce_success;
    thread_node_t *threads_base = threads;
    while (!sock_server_listen_connection(&server->sock_server, &threads->client_args.client))
    {
        threads->next = malloc(sizeof(thread_node_t));
        if (threads->next == NULL)
        {
            status = rpce_resource;
            break;
        }

        threads = threads->next;
        threads->next = NULL;

        threads->client_args.al = &server->config.access_list;
        if (thrd_create(&threads->thread, &rpc_client_handle, &threads->client_args) != thrd_success)
        {
            status = rpce_resource;
            break;
        }
    }

    linked_list_delete(threads_base, &thread_node_destructor);
    return status;
}

void rpc_server_stop(rpc_server_t *server)
{
    server->sock_server._stop_listening = 1;
}

void rpc_server_close(rpc_server_t *server)
{
    rpc_server_stop(server);
    sock_server_close(&rpc_server_static->sock_server);
    linked_list_delete(&server->config.access_list, ll_destructor_default);
}
