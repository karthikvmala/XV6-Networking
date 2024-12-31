#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char process[30][100] = {"test", "fork", "exit", "wait", "pipe", "read", "kill", "exec", "fstat", "chdir", "dup", "getpid", "sbrk", 
"sleep", "uptime", "open", "write", "mknod", "unlink", "link", "mkdir", "close", "waitx", "getSyscount", "sigalarm", "sigretrun",
"settickets", "init_priority", "test", "test"};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mask> <command> [args...]\n", argv[0]);
        exit(1);
    }

    int mask = atoi(argv[1]);
    int n = 0;
    while (mask / 2) {
        n++;
        mask /= 2;
    }

    int pd = fork();
    getSysCount(-1);
    if (pd == -1) {
        printf("Error: fork failed\n");
        exit(1);
    } else if (pd == 0) {
        exec(argv[2], &argv[2]);
        printf("Error: exec failed\n");
        exit(1);
    } else {
        wait(0);
    }
    
    printf("PID %d called %s %d times\n", pd, process[n], getSysCount(n));
    return 0;
}