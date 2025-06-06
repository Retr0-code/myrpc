#pragma once
#include <stdint.h>
#include <sys/socket.h>

#include "sockutils.h"

typedef struct
{
    sockaddr_u _address;
    socklen_t  _addr_len;
    int        _socket_descriptor;
    int        _use_ipv6;
} sock_client_t;

int sock_client_create(
    sock_client_t *client,
    const char *rhost,
    const char *rport,
    int use_ipv6,
    int sock_type
);

int sock_client_connect(sock_client_t *client);

void sock_client_stop(sock_client_t *client);
