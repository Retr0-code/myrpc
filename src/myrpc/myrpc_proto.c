#include <stdlib.h>

#include "myrpc/protocol.h"
#include "myrpc/myrpc_proto.h"
#include "myrpc_proto.h"

int rpc_receive_username(int fd, char **username)
{
    return message_receive(fd, mt_username, 0, username);
}

int rpc_send_username(int fd, const char *username)
{
    return message_send(fd, mt_username, strlen(username) + 1, username);
}

int rpc_receive_command(int fd, char **command)
{
    return message_receive(fd, mt_request, 0, command);
}

int rpc_send_command(int fd, const char *command, size_t length)
{
    return message_send(fd, mt_request, length, command);
}

int rpc_receive_output(int fd, char **output)
{
    return message_receive(fd, mt_response, 0, output);
}

int rpc_send_output(int fd, const char *output, size_t length)
{
    return message_send(fd, mt_response, length, output);
}

int rpc_send_close(int fd, int rpce_reason)
{
    return message_send(fd, mt_close, sizeof(rpce_reason), &rpce_reason);
}
