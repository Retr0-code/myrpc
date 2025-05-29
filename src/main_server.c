#include <errno.h>
#include <stdio.h>

#include "myrpc/myrpc_server.h"

#define PID_FILE    "/run/myrpcd.pid"
#define CONFIG_FILE "/etc/myRPC/myRPC.conf"

void daemonize(void);

int main(int argc, char **argv)
{
    rpc_server_t myrpc;

    if (rpc_server_read_config(&myrpc.config, NULL) != rpce_success)
        return -1;

    if (rpc_server_create(&myrpc, &myrpc.config))
        return -1;

    rpc_server_run(&myrpc);
    return 0;
}

void daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0)
        exit(-1);
    else if (pid > 0)
        exit(0);

    if (setsid() == -1)
        exit(errno);

    pid = fork();
    if (pid < 0)
        exit(-1);
    else if (pid > 0)
    {
        FILE *pidfile = fopen(PID_FILE, "w");
        if (pidfile == NULL)
            exit(-1);

        fprintf(pidfile, "%i", pid);
        fclose(pidfile);

        exit(0);
    }

    chdir("/");

    for (int fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
        close(fd);

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w");
    stderr = fopen("/dev/null", "w");
}
