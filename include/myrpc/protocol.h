#pragma once
#include <stdint.h>
#include <stddef.h>

enum message_type_e
{
    mt_close,
    mt_username,
    mt_request,
    mt_response,
    mt_serialization
};

typedef struct
{
    int32_t  type;
    uint32_t length;
    char     *body;
} __attribute__((aligned(4), packed)) net_message_t;

enum message_error_e
{
    me_success,
    me_invalid_args,
    me_wrong_type,
    me_wrong_length,
    me_bad_socket,
    me_send,
    me_receive,
    me_peer_end,
    me_resource
};

const char *me_strerror(int status);

int message_send(int socket_fd, int type, size_t data_length, const char *data);

int message_receive(int socket_fd, int type, size_t data_length, char **data);
