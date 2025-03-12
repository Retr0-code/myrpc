#pragma once
#include "status.h"

typedef enum
{
    socket_error_success,
    socket_error_init,
    socket_error_bind,
    socket_error_listen,
    socket_error_domain,
    socket_error_connect,
    socket_error_transmission,
    socket_error_invalid_args
} network_exceptions;
