#include <stdlib.h>

#include "server/server.h"
#include "network_exceptions.h"

#define USE_IPv6 1

#ifdef USE_IPv6
#define HOST    "::1%lo"
#else
#define HOST    "127.0.0.1"
#endif
#define PORT    8080

#define BUFFER_SIZE 4096

int main()
{
    client_interface_t client;
    sock_server_t server;
    if (sock_server_create(&server, HOST, PORT, USE_IPv6) != socket_error_success)
    {
        fprintf(stderr, "%s Failed to create server\n", ERROR);
        return -1;
    }

    int just_started = 1;
    char msg[BUFFER_SIZE];
    while (just_started || !sock_server_listen_connection(&server, &client))
    {
        just_started = 0;
        memset(msg, 0, BUFFER_SIZE);
        if (read(client._socket_descriptor, msg, BUFFER_SIZE) != 0)
            printf("Client sent message: %s\n", msg);

        if (strncmp("close\n", msg, 6) == 0)
            client_interface_close_connection(&client);
    }

    sock_server_close(&server);

    return 0;
}
