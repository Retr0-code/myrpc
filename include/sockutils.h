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

int socket_fill_sockaddr_in(sockaddr_u *addr, const char *lhost, in_port_t lport);

int socket_fill_sockaddr_in6(sockaddr_u *addr, const char *lhost, in_port_t lport);

int socket_bind(sockaddr_u *addr, int use_ipv6, int socket_fd, const char *lhost, in_port_t lport);

void socket_get_address(char *buffer, sockaddr_u *addr, int use_ipv6);

void socket_shutdown_close(int socket_fd);
