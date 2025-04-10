#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <threads.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

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
    mqd_t command_queue = mq_open("/mq_209910", O_RDONLY);
    //printf("I am %d, child of parent process %d\n", getpid(), getppid());

    printf("[%d] Doing some work for %d...\n", getpid(), getppid());
    sleep(instructions.work);
    printf("[%d] Jobs done!\n", getpid());
    printf("[%d] bringing some fish to %d\n", getpid(), getppid());

    
    return getpid();
}
/*
int individual_child(void* _arg) {
    srand(getpid());
    int const work = rand() % 5;
    printf("%d Doing some work for %d\n", getpid(), getppid());
    sleep(work);
    printf("[%d] Jobs done!\n", getpid());

    return work;
}

int child_labour() {
    thrd_t children[10];
    for (int i = 0; i < 10; ++i) {
        thrd_create(&children[i], individual_child, NULL);
    }
    int fishes = 0;
    for (int i = 0; i < 10; ++i) {
        int result = 0;
        thrd_join(children[i], &result);
    }

    return getpid();
}
*/
struct work_message {
    int work;
};

int main(int argc, char * argv[]) {
    //cli_args const args = parse_command_line(argc, argv);
    //printf("i = %d, s: %s, b: %d\n", args.i, args.s, args.b);

    //printf("PID: %d\n", getpid());
    struct mq_atrr queue_options = {
        .mq_maxmsg = 1,
        .mq_msgsize = sizeof(struct work_message),
    };

    mqd_t command_queue = mq_open("/mq_209910", O_WRONLY | O_CREAT , S_IRWXU, &queue_options);


    if (command_queue == -1) {
        fprintf(stderr,"Failed to open the Queue\n");

        return EXIT_FAILURE;
    }

    struct work_message instructions;
    int const received = mq_receive(command_queue, (void*)&instructions, sizeof(struct work_message), NULL );
    if (received == -1) {
        return EXIT_FAILURE;
    }
    
    //printf("[%d] mq_open returned %d\n", getpid(), command_queue);

    printf("[%d] Sending a child into the sea . . . . . \n", getpid());
    for (int i = 0; i <10; i++) {
        pid_t forked = fork();

        if (forked == 0) {
            return do_work();
        }
    }
    
    struct work_message instructions = { .work = 5};
    int sent = mq_send(command_queue, (void*)&instructions, sizeof(struct work_message), 0);
   
    if (sent == -1) {
        fprintf(stderr, "Failed");
        return EXIT_FAILURE;
    }
    printf("[%d] Enjoy some food . . . . . \n", getpid());
    for(int i = 0; i < 10; ++i) {
        int wstatus = 0;
        pid_t const waited = wait(&wstatus);
        if(WIFEXITED(wstatus)) {
            printf("%d child %d exited normally with return code %d\n", getpid(), waited, WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)){
            printf("%d Child %d terminated with signal %d\n", getpid(), waited, WTERMSIG(wstatus));
        } else {
            printf("%d Child %d is terminated not normally\n", getpid(), waited);
        }
    }
    mq_close(command_queue);
    //mq_unlink("/mq_209910");
    

    printf("All children are back\n");
    //printf("[%d] wait returned %d, status is %d\n", getpid(), waited, wstatus);

    //printf("PID: %d, fork returned %d, i am child of %d\n", getpid(),forked, getppid());

    return 0;
} 