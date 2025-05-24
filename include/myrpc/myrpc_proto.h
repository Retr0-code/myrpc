#pragma once
#include "myrpc/protocol.h"

int rpc_receive_username(int fd, char **username);

int rpc_send_username(int fd, const char *username);

int rpc_receive_command(int fd, char **command);

int rpc_send_command(int fd, const char *command, size_t length);

int rpc_receive_output(int fd, char **output);

int rpc_send_output(int fd, const char *output, size_t length);

int rpc_send_close(int fd, int rpce_reason);
