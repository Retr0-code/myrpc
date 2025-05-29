#pragma once
#include <net/if.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef union
{
    struct sockaddr_in  addr_v4;
    struct sockaddr_in6 addr_v6;
} sockaddr_u;

int socket_resolve_addr(sockaddr_u *addr,
                socklen_t *addr_len,
                int socktype,
                int use_ipv6,
                const char *lhost,
                const char *lport);

void socket_get_address(char *buffer, sockaddr_u *addr, int use_ipv6);

void socket_shutdown_close(int socket_fd);

int str_to_socket_type(const char *str_socktype);
