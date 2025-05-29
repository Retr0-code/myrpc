#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "client/client.h"
#include "myrpc/myrpc_proto.h"
#include "network_exceptions.h"

#define HOST "0.0.0.0"
#define PORT "135"

static struct option arguments_long[] =
    {
        {"host", required_argument, NULL, 'h'},
        {"port", required_argument, NULL, 'p'},
        {"user", required_argument, NULL, 'u'},
        {"command", required_argument, NULL, 'c'},
        {"socktype", optional_argument, NULL, 's'},
        {"ipv6", no_argument, NULL, '6'},
        {"help", no_argument, NULL, 'm'}};
const char *arguments = "h:p:u:c:s:6m";

void show_help(const char *name)
{
    printf("Usage: %s [PARAMETERS] <-c | --command=> \"<shell command>\"\n", name);
    puts("--host\t-h\tspecifies server address.");
    puts("--port\t-p\tspecifies server port.");
    puts("--user\t-u\tspecifies username.");
    puts("--command\t-c\tspecifies command sent to the server.");
    puts("--socktype\t-s\tspecifies protocol <SOCK_STREAM | SOCK_DGRAM>.");
    puts("--ipv6\t-6\tspecifies which ip version to use (flag).");
    puts("--help\t-m\tshows help menu.");
}

int main(int argc, char **argv)
{
    int arg_index = 0;
    int option = 0;
    int flag = 0;

    struct
    {
        char host[INET6_ADDRSTRLEN];
        char port[6];
        char *user;
        char *command;
        int socktype;
        int use_ipv6;
    } client_parameters = {HOST, PORT, "root", "pwd", SOCK_STREAM, 0};

    while ((option = getopt_long(argc, argv, arguments, arguments_long, &arg_index)) != -1)
    {
        if (flag & (1 << arg_index))
            continue;

        switch (option)
        {
        case 'p':
        {
            if (optarg)
                strncpy(client_parameters.port, optarg, 6);
            break;
        }

        case 'h':
        {
            if (optarg)
                strncpy(client_parameters.host, optarg, INET6_ADDRSTRLEN);
            break;
        }

        case 's':
        {
            int sock_type = str_to_socket_type(optarg);
            client_parameters.socktype = sock_type == -1 ? client_parameters.socktype : sock_type;
            break;
        }

        case '6':
        {
            client_parameters.use_ipv6 = 1;
            break;
        }

        case 'u':
        {
            if (optarg == NULL)
                break;

            size_t length = strlen(optarg) + 1;
            client_parameters.user = malloc(length);
            strncpy(client_parameters.user, optarg, length);
            break;
        }

        case 'c':
        {
            if (optarg == NULL)
                break;

            size_t length = strlen(optarg) + 1;
            client_parameters.command = malloc(length);
            strncpy(client_parameters.command, optarg, length);
            break;
        }

        case 'm':
        {
            show_help(argv[0]);
            exit(0);
        }

        default:
            break;
        }
        flag |= (1 << arg_index);
    }

    sock_client_t client;
    if (sock_client_create(&client,
                           client_parameters.host,
                           client_parameters.port,
                           client_parameters.use_ipv6,
                           client_parameters.socktype) != socket_error_success)
        return -1;

    if (sock_client_connect(&client) != socket_error_success)
        return -1;

    if (rpc_send_username(client._socket_descriptor, client_parameters.user) != me_success)
        return -1;

    if (rpc_send_command(client._socket_descriptor, client_parameters.command) != me_success)
        return -1;

    rpc_output_t output;
    if (rpc_receive_output(client._socket_descriptor, &output) != me_success)
        return -1;

    printf("output:\n%s\ncommand executed with status: %i\n", output.output, output.status);
    sock_client_stop(&client);
    return 0;
}
