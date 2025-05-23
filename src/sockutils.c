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

int socket_fill_sockaddr_in(sockaddr_u *addr, const char *lhost, in_port_t lport)
{
    addr->addr_v4.sin_family = AF_INET;
    addr->addr_v4.sin_port = htons(lport);
    if (inet_pton(AF_INET, lhost, &addr->addr_v4.sin_addr) != 1)
        return socket_error_invalid_args;

    return socket_error_success;
}

static uint32_t sock_server_get_scope_id(int socket_fd, const char *interface_name)
{
    struct ifreq interface_descriptor;
    interface_descriptor.ifr_addr.sa_family = AF_INET;
    strncpy(interface_descriptor.ifr_name, interface_name, IFNAMSIZ - 1);

    // Get interface general info
    ioctl(socket_fd, SIOCGIFADDR, &interface_descriptor);
    return interface_descriptor.ifr_ifru.ifru_ivalue;
}

int socket_fill_sockaddr_in6(sockaddr_u *addr, const char *lhost, in_port_t lport)
{
    char ipv6[INET6_ADDRSTRLEN];
    uint8_t delimeter_index = strcspn(lhost, "%");
    strncpy(ipv6, lhost, delimeter_index);
    const char *scope_id = lhost + delimeter_index;

    addr->addr_v6.sin6_family = AF_INET6;
    addr->addr_v6.sin6_port = htons(lport);

    if (inet_pton(AF_INET6, ipv6, &addr->addr_v6.sin6_addr) != 1)
        return socket_error_invalid_args;

    addr->addr_v6.sin6_scope_id = atoi(scope_id);
    addr->addr_v6.sin6_scope_id = sock_server_get_scope_id(0, scope_id);

    return socket_error_success;
}

int socket_bind(sockaddr_u *addr, int use_ipv6, int socket_fd, const char *lhost, in_port_t lport)
{
    typedef int (*fill_sockaddr_ptr)(sockaddr_u *, const char *, in_port_t);

    fill_sockaddr_ptr bind_func = use_ipv6 ? &socket_fill_sockaddr_in6 : &socket_fill_sockaddr_in;

    if ((bind_func)(addr, lhost, lport) != socket_error_success)
        return socket_error_bind;

    if (bind(socket_fd,
            (struct sockaddr *)addr,
            use_ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) != 0)
        return socket_error_bind;
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
        fprintf(stderr, "%s Shuting down client:\t%s\n", WARNING, strerror(errno));
    
    if (close(socket_fd) != 0)
        fprintf(stderr, "%s Closing client:\t%s\n", WARNING, strerror(errno));
}
