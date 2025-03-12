#include <stdlib.h>

#include "server/server.h"
#include "network_exceptions.h"

#ifdef USE_IPv6
#define HOST    "::1%lo"
#else
#define HOST    "127.0.0.1"
#endif
#define PORT    8080

#define BUFFER_SIZE 4096

int main()
{
    Server *server = malloc(sizeof(Server));
    if (Server_create(server, HOST, PORT, 0, 100) != socket_error_success)
    {
        fprintf(stderr, "%s Failed to create server\n", ERROR);
        return -1;
    }

    // if (Server_listen(server) != thrd_success)
    // {
    //     fprintf(stderr, "%s Starting listener\n", ERROR);
    //     return -1;
    // }

    // int just_started = 1;
    // char msg[BUFFER_SIZE];
    // while (just_started || server->_clients_amount)
    // {
    //     if (server->_clients_amount > 0)
    //     {
    //         just_started = 0;
    //         memset(msg, 0, BUFFER_SIZE);
    //         if (read(server->_clients[0]->_socket_descriptor, msg, BUFFER_SIZE) != 0)
    //             printf("Client sent message: %s\n", msg);

    //         if (strncmp("close\n", msg, 6) == 0)
    //             ClientInterface_close_connection(server->_clients[0]);
    //     }
    // }

    Server_close(server);

    return 0;
}
