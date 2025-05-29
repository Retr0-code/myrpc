#include "list.h"
#include "myrpc/myrpc_proto.h"
#include "myrpc/myrpc_server.h"
#include "network_exceptions.h"

#include "mysyslog.h"

#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <threads.h>
#include <sys/wait.h>

#define LOG_DRIVER drv_json
#define LOG_FORMAT fmt_json
#define LOG_PATH "/var/log/myrpc"
#define mysyslog_server(msg, lvl) mysyslog(msg, lvl, LOG_DRIVER, LOG_FORMAT, LOG_PATH);

#ifndef RPC_DEFAULT_CONFIG
#define RPC_DEFAULT_CONFIG
rpc_server_config_t default_config = {
    {NULL, 0},
    "127.0.0.1",
    "135",
    SOCK_STREAM,
    1};
#endif

#define TMP_DIR "/tmp"
#define TMP_PREFIX TMP_DIR "/myRPC_"
#define TMP_STDOUT TMP_PREFIX "XXXXXX.stdout"
#define TMP_STDERR TMP_PREFIX "XXXXXX.stderr"
#define TMP_SUFFIX_LEN 7

static rpc_server_t *rpc_server_static = NULL;

static struct option arguments[] = {
    {"port", required_argument, 0, 'p'},
    {"host", required_argument, 0, 'h'},
    {"socktype", required_argument, 0, 's'},
    {"use_ipv6", required_argument, 0, '6'},
    {"access_list", required_argument, 0, 'a'}};
static const int required_argc = sizeof(arguments) / sizeof(struct option) + 1;

static void signal_handler_stop(int signal)
{
    rpc_server_close(rpc_server_static);
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
    rpc_access_list_t *prev = al;
    while (getline(&username, &capacity, al_file) != -1)
    {
        username[strlen(username) - 1] = 0;
        struct passwd *user = getpwnam(username);
        if (user == NULL)
            continue;

        al->uid = user->pw_uid;
        al->next = malloc(sizeof(rpc_access_list_t));
        prev = al;
        al = al->next;
    }
    free(prev->next);
    prev->next = NULL;

    free(username);
    fclose(al_file);
    return rpce_success;
}

static int rpc_parse_config(rpc_server_config_t *config, char **argv, int argc)
{
    int arg_index = 0;
    int option = 0;
    int flag = 0;

    while ((option = getopt_long(argc, argv, "p:h:s:6:a:", arguments, &arg_index)) != -1)
    {
        if (flag & (1 << arg_index))
            continue;

        switch (option)
        {
        case 'p':
        {
            if (optarg)
                strncpy(config->port, optarg, 6);

            break;
        }

        case 'h':
        {
            if (optarg)
                strncpy(config->address, optarg, INET6_ADDRSTRLEN);

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
            config->use_ipv6 = atoi(optarg) > 0;
            break;
        }

        case 'a':
        {
            if (rpc_read_access_list(optarg, &config->access_list) != rpce_success)
            {
                mysyslog_server("Problem reading access list", loglvl_ERROR);
                return rpce_user;
            }

            break;
        }

        default:
            break;
        }
        flag |= (1 << arg_index);
    }

    mysyslog_server("Read config and access list", loglvl_INFO);
    return rpce_success;
}

static char *remove_spaces(char *str)
{
    if (str == NULL)
        return str;

    while (*str == ' ' || *str == '\t') {
        ++str;
    }
    return str;
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
    while (getline(&line, &length, config_file) != -1)
    {
        char *arg_line_start = remove_spaces(line);
        if (*arg_line_start == '#')
            continue;

        arg_line_start = strtok(arg_line_start, "#");

        ++argc;
        size_t line_length = strlen(arg_line_start);
        arg_line_start[line_length - 1] = 0;
        argv[argc] = malloc(line_length + 2);
        argv[argc][0] = '-';
        argv[argc][1] = '-';
        memcpy(argv[argc] + 2, arg_line_start, line_length);
    }
    ++argc;
    free(line);
    fclose(config_file);

    rpc_parse_config(config, argv, argc);

    for (--argc; argc != 1; --argc)
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
        mysyslog_server("Failed to create server", loglvl_CRITICAL);
        return status;
    }

    signal(SIGINT, &signal_handler_stop);
    signal(SIGTERM, &signal_handler_stop);
    mysyslog_server("Successfully initialized RPC server", loglvl_INFO);

    return rpce_success;
}

static int rpc_user_in_list(const rpc_access_list_t *al, uid_t uid)
{
    for (; al != NULL; al = al->next)
        if (al->uid == uid)
            return rpce_success;

    return rpce_user;
}

static inline void add_tempfd_flags(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    flags |= O_APPEND; // | ...
    fcntl(fd, F_SETFL);
}

static int rpc_try_exec(
    uid_t uid,
    const rpc_command_t *command,
    rpc_output_t *output)
{
    if (command == NULL || output == NULL)
    {
        errno = EINVAL;
        return rpce_invalid_args;
    }

    char tmpname_stdout[] = TMP_STDOUT;
    int tmpfd_stdout = mkstemps(tmpname_stdout, TMP_SUFFIX_LEN);
    if (tmpfd_stdout == -1)
    {
        mysyslog_server("Unable to create STDOUT temporary file", loglvl_CRITICAL);
        return rpce_resource;
    }

    char tmpname_stderr[] = TMP_STDERR;
    int tmpfd_stderr = mkstemps(tmpname_stderr, TMP_SUFFIX_LEN);
    if (tmpfd_stderr == -1)
    {
        mysyslog_server("Unable to create STDERR temporary file", loglvl_CRITICAL);
        close(tmpfd_stdout);
        return rpce_resource;
    }

    add_tempfd_flags(tmpfd_stdout);
    add_tempfd_flags(tmpfd_stderr);

    pid_t cpid = fork();
    switch (cpid)
    {
    case 0:
    {
        if (setuid(uid) == -1)
        {
#define ERR_MSG "Unable to get UID:%i"
#define ERR_MSG_LEN sizeof(ERR_MSG)
#define MAX_UID_LEN 5
            char err[ERR_MSG_LEN + MAX_UID_LEN];
            sprintf(err, ERR_MSG, uid);
            mysyslog_server(err, loglvl_CRITICAL);
            exit(rpce_privilege);
        }

        dup2(tmpfd_stdout, STDOUT_FILENO);
        dup2(tmpfd_stderr, STDERR_FILENO);
        close(tmpfd_stdout);
        close(tmpfd_stderr);

        if (execvp(command->argv[0], command->argv) == -1)
            exit(rpce_command);

        exit(rpce_success);
    }

    case -1:
        close(tmpfd_stdout);
        close(tmpfd_stderr);
        return rpce_resource;

    default:
        waitpid(cpid, &output->status, 0);
        break;
    }

    int outputfd = tmpfd_stdout;
    if (output->status)
        outputfd = tmpfd_stderr;

    while (1)
    {
        off_t filesize = lseek(outputfd, 0, SEEK_END);
        if (filesize == -1 || lseek(outputfd, 0, SEEK_SET) == -1)
        {
            output->status = rpce_resource;
            break;
        }

        char *filebuffer = malloc(filesize);
        if (filebuffer == NULL)
        {
            output->status = rpce_resource;
            break;
        }

        if (read(outputfd, filebuffer, filesize) == -1)
        {
            free(filebuffer);
            output->status = rpce_resource;
            break;
        }

        output->output = filebuffer;
        output->length = filesize;
        break;
    }

    mysyslog_server("Executed a command", loglvl_INFO);
    close(tmpfd_stdout);
    close(tmpfd_stderr);
    return rpce_success;
}

static int rpc_client_handle(client_args_t *args)
{
    char *username = NULL;
    if (rpc_receive_username(args->client._socket_descriptor, &username) != me_success)
    {
        mysyslog_server("Unable to receive username from client", loglvl_ERROR);
        return rpce_user;
    }

    struct passwd *record = getpwnam(username);
    if (record == NULL)
    {
        mysyslog_server("Unable to get user's passwd record", loglvl_ERROR);
        rpc_send_close(args->client._socket_descriptor, rpce_user);
        return rpce_user;
    }

    if (rpc_user_in_list(args->al, record->pw_uid) != rpce_success)
    {
        mysyslog_server("User not in access list", loglvl_ERROR);
        rpc_send_close(args->client._socket_descriptor, rpce_user);
        return rpce_user;
    }

    rpc_command_t command;
    if (rpc_receive_command(args->client._socket_descriptor, &command) != me_success)
    {
        mysyslog_server("User not in access list", loglvl_ERROR);
        return rpce_command;
    }

    rpc_output_t output = {NULL, 0, 0};
    if (rpc_try_exec(record->pw_uid, &command, &output) != rpce_success)
    {
        mysyslog_server("Unable to execute a command", loglvl_ERROR);
        rpc_send_close(args->client._socket_descriptor, rpce_command);
        return rpce_command;
    }

    // Send output
    if (rpc_send_output(args->client._socket_descriptor, &output) != me_success)
    {
        mysyslog_server("Unable to send the command's output to the client", loglvl_ERROR);
        return rpce_output;
    }

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
    mysyslog_server("Started RPC server", loglvl_INFO);
    while (!sock_server_listen_connection(&server->sock_server, &threads->client_args.client))
    {
        threads->client_args.al = &server->config.access_list;
        if (thrd_create(&threads->thread, &rpc_client_handle, &threads->client_args) != thrd_success)
        {
            mysyslog_server("Unable to start client thread", loglvl_CRITICAL);
            status = rpce_resource;
            break;
        }

        threads->next = malloc(sizeof(thread_node_t));
        if (threads->next == NULL)
        {
            mysyslog_server("Unable to create new thread record for the next client", loglvl_CRITICAL);
            status = rpce_resource;
            break;
        }

        threads = threads->next;
        threads->next = NULL;
    }

    linked_list_delete((linked_list_t *)threads_base, &thread_node_destructor);
    return status;
}

void rpc_server_stop(rpc_server_t *server)
{
    mysyslog_server("Stopping RPC server", loglvl_WARN);
    server->sock_server._stop_listening = 1;
}

void rpc_server_close(rpc_server_t *server)
{
    rpc_server_stop(server);
    mysyslog_server("Shutting down RPC server", loglvl_WARN);
    sock_server_close(&rpc_server_static->sock_server);
    linked_list_delete((linked_list_t *)server->config.access_list.next, ll_destructor_default);
}
