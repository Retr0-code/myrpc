#include <stdlib.h>
#include <string.h>

#include "myrpc/protocol.h"
#include "myrpc/myrpc_proto.h"

// MAX_CMD_LEN = 8 kB
#define MAX_CMD_LEN (1 << 13)

int rpc_receive_username(int fd, char **username)
{
    return message_receive(fd, mt_username, 0, username);
}

int rpc_send_username(int fd, const char *username)
{
    return message_send(fd, mt_username, strlen(username) + 1, username);
}

int rpc_receive_command(int fd, rpc_command_t *command)
{
    /*
        struct packed_command
        {
            int argc;
            size_t argv_len[argc];
            char *argv[argc];
        }
    */
    char *packed_command = NULL;
    int status = message_receive(fd, mt_request, 0, &packed_command);
    if (status != me_success)
    {
        free(packed_command);
        return status;
    }

    command->argc = *(int *)packed_command;
    command->argv_len = malloc((command->argc + 1) * sizeof(size_t));
    if (command->argv_len == NULL)
    {
        free(packed_command);
        return me_resource;
    }
    command->argv = malloc((command->argc + 1) * sizeof(char *));
    if (command->argv == NULL)
    {
        free(packed_command);
        free(command->argv_len);
        return me_resource;
    }

    memcpy(command->argv_len, packed_command + sizeof(int), command->argc * sizeof(size_t));
    char *argv_packed = packed_command + sizeof(int) + command->argc * sizeof(size_t);
    for (size_t i = 0; i < command->argc; ++i)
    {
        command->argv[i] = malloc(command->argv_len[i]);
        strncpy(command->argv[i], argv_packed, command->argv_len[i]);
        argv_packed += command->argv_len[i];
    }
    command->argv_len[command->argc] = 0;
    command->argv[command->argc] = NULL;

    free(packed_command);
    return status;
}

int rpc_send_command(int fd, const char *command)
{
    int status = me_success;
    size_t length = strnlen(command, MAX_CMD_LEN + 1);
    if (length > MAX_CMD_LEN)
        return me_invalid_args;

    ++length;

    int argc = 1;
    for (size_t i = 0; i < length; ++i)
        argc += (command[i] == ' ');

    /*
    struct packed_command
    {
        int argc;
        size_t argv_len[argc];
        char *argv[argc];
    }
    */
    size_t total_length = sizeof(int) + sizeof(size_t) * argc + length;
    char *packed_command = malloc(total_length);
    if (packed_command == NULL)
        return me_resource;

    size_t *cmd_size = packed_command + sizeof(int);
    char *argv = packed_command + sizeof(int) + sizeof(size_t) * argc;
    *(int *)packed_command = argc;
    memcpy(argv, command, length);
    if (argc == 1)
    {
        *cmd_size = length;
        status = message_send(fd, mt_request, total_length, packed_command);
        free(packed_command);
        return status;
    }

    size_t prev_end = 0;
    size_t arg_pos = 0;
    for (size_t i = 0; i < length; ++i)
    {
        if (argv[i] == ' ' || argv[i] == 0)
        {
            argv[i] = 0;
            cmd_size[arg_pos++] = strlen(argv + prev_end) + 1;
            prev_end = i + 1;
        }
    }

    status = message_send(fd, mt_request, total_length, packed_command);
    free(packed_command);
    return status;
}

int rpc_receive_output(int fd, rpc_output_t *output)
{
    /*
    struct packed_output
    {
        int status;
        size_t length;
        char output[length];
    }
    */
    char *packed_output = NULL;

    int status = message_receive(fd, mt_response, 0, &packed_output);
    if (status != me_success)
    {
        free(packed_output);
        return status;
    }

    output->status = *(int*)packed_output;
    output->length = *(size_t*)(packed_output + sizeof(int));
    output->output = malloc(++output->length);
    if (output->output == NULL)
    {
        free(packed_output);
        return me_resource;
    }
    
    strncpy(output->output, packed_output + sizeof(int) + sizeof(size_t), output->length);
    output->output[output->length - 1] = 0;
    return status;
}

int rpc_send_output(int fd, const rpc_output_t *output)
{
    /*
    struct packed_output
    {
        int status;
        size_t length;
        char output[length];
    }
    */
    size_t total_length = output->length + 1 + sizeof(int) + sizeof(size_t);
    char *packed_output = malloc(total_length);
    *(int *)packed_output = output->status;
    size_t *length = packed_output + sizeof(int);
    char *output_text = packed_output + sizeof(int) + sizeof(size_t);

    *length = output->length;
    strncpy(output_text, output->output, output->length);

    return message_send(fd, mt_response, total_length, packed_output);
}

int rpc_send_close(int fd, int rpce_reason)
{
    return message_send(fd, mt_close, sizeof(rpce_reason), &rpce_reason);
}
