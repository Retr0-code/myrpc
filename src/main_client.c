#include <stdio.h>
#include <stdlib.h>

#include "client/client.h"
#include "myrpc/myrpc_proto.h"
#include "network_exceptions.h"

#define HOST "127.0.0.1"
#define PORT 135

int main(int argc, char **argv)
{
    sock_client_t client;
    if (sock_client_create(&client, HOST, PORT, 0, SOCK_STREAM) != socket_error_success)
        return -1;

    if (sock_client_connect(&client) != socket_error_success)
        return -1;

    if (rpc_send_username(client._socket_descriptor, "root") != me_success)
        return -1;

    if (rpc_send_command(client._socket_descriptor, "ls -la") != me_success)
        return -1;

    rpc_output_t output;
    if (rpc_receive_output(client._socket_descriptor, &output) != me_success)
        return -1;

    printf("output:\n%s\ncommand executed with status: %i\n", output.output, output.status);
    sock_client_stop(&client);
    return 0;
}
