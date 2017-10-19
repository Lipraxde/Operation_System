#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid = fork();
    
    if(pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        printf("[ Child  ]: Parent PID = %d\n", getppid());
        printf("[ Child  ]: MY     PID = %d\n", getpid());
        _exit(EXIT_SUCCESS);
    }
    else
    {
        int status;
        printf("[ Parent ]: MY     PID = %d\n", getpid());
        printf("[ Parent ]: Child  PID = %d\n", pid);
        (void)waitpid(pid, &status, 0);
    }
    return 0;
}

