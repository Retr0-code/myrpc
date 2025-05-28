#pragma once
#include <stddef.h>
#include <stdint.h>

#include "myrpc/protocol.h"

typedef struct
{
    size_t *argv_len;
    char **argv;
    int argc;
} rpc_command_t;

typedef struct
{
    char *output;
    size_t length;
    int status;
} rpc_output_t;

int rpc_receive_username(int fd, char **username);

int rpc_send_username(int fd, const char *username);

int rpc_receive_command(int fd, rpc_command_t *command);

int rpc_send_command(int fd, const char *command);

int rpc_receive_output(int fd, rpc_output_t *output);

int rpc_send_output(int fd, const rpc_output_t *output);

int rpc_send_close(int fd, int rpce_reason);
