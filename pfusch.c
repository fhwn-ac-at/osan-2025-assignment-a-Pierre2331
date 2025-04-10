#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

typedef struct command_line_arguments {
    int i;
    char const *s;
    bool b;
} cli_args;

cli_args parse_command_line(int const argc, char * argv[]) {
    cli_args args = {0, NULL, false};

    int optgot = -1;

    do {
        optgot = getopt(argc, argv, "i:s:b");
        switch (optgot)
        {
        case 'i':
            args.i = atoi(optarg);
            break;
        case 's':
            args.s = optarg;
            break;
        case 'b':
           args.b = true;
            break;

        case '?':
            exit(EXIT_FAILURE);
        }
    } while (optgot != -1);

    return args;
}

int do_work() {
    printf("I am %d, child of parent process %d\n", getpid(), getppid());

    printf("[%d], doing some work for %d \n", getpid(), getppid());
    sleep(3);
    printf("[%d] Jobs done!", getpid());
    printf("[%d] bringing some fish to %d\n", getpid(), getppid());

    //*(int*)(12) = 0;
    return getpid();
}

int main(int argc, char * argv[]) {
    //cli_args const args = parse_command_line(argc, argv);
    //printf("i = %d, s: %s, b: %d\n", args.i, args.s, args.b);

    //printf("PID: %d\n", getpid());

    printf("[%d] Sending a child into the sea . . . . . \n", getpid());

    pid_t forked = fork();

    if (forked == 0) {
        return do_work();
    }

    printf("[%d] Enjoy some food . . . . . \n", getpid());

    int wstatus = 0;
    pid_t const waited = wait(&wstatus);
    if(WIFEXITED(wstatus)) {
        printf("%d child %d exited normally with return code %d\n", getpid(), waited, WEXITSTATUS(wstatus) )
    } else if (WIFSIGNALED(wstatus)){
        printf("%d Child %d terminated with signal %d\n", getpid(), waited, WTERMSIG(wstatus));
    } else {
        printf("%d Child %d is terminated not normally\n", getpid(), waited());
    }

    printf("All children are back\n");
    //printf("[%d] wait returned %d, status is %d\n", getpid(), waited, wstatus);

    //printf("PID: %d, fork returned %d, i am child of %d\n", getpid(),forked, getppid());

    return 0;
} 