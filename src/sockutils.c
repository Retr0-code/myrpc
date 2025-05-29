#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "sockutils.h"
#include "network_exceptions.h"

int socket_resolve_addr(sockaddr_u *addr, socklen_t *addrlen, int socktype, int use_ipv6, const char *lhost, const char *lport)
{
    typedef int (*fill_sockaddr_ptr)(sockaddr_u *, const char *, in_port_t);
    struct addrinfo hints, *resolved;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = use_ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = socktype;

    int status = 0;
    if ((status = getaddrinfo(lhost, lport, &hints, &resolved)) != 0)
    {
        fprintf(stderr, "%s Error getting address info:\t%s\n", ERROR, gai_strerror(status));
        errno = status;
        return socket_error_bind;
    }
    memcpy(addr, resolved->ai_addr, resolved->ai_addrlen);
    *addrlen = resolved->ai_addrlen;

    freeaddrinfo(resolved);
    return socket_error_success;
}

void socket_get_address(char *buffer, sockaddr_u *addr, int use_ipv6)
{
    if (use_ipv6)
        inet_ntop(AF_INET6, &addr->addr_v6.sin6_addr, buffer, sizeof(addr->addr_v6));
    else
        inet_ntop(AF_INET, &addr->addr_v4.sin_addr, buffer, sizeof(addr->addr_v4));
}

void socket_shutdown_close(int socket_fd)
{
    if (shutdown(socket_fd, SHUT_RDWR) != 0)
        fprintf(stderr, "%s Shuting down socket:\t%s\n", WARNING, strerror(errno));

    if (close(socket_fd) != 0)
        fprintf(stderr, "%s Closing socket:\t%s\n", WARNING, strerror(errno));
}

int str_to_socket_type(const char *str_socktype)
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
