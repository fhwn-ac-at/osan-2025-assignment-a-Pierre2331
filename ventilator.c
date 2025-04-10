#include <mqueue.h>   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>


typedef struct {
    int workers;
    int tasks;
    int queue_size;
} config_t;

typedef struct {
    int worker_id;
    pid_t pid;
    int task_count;
    int total_sleep_time;
} result_t;

config_t parse_arguments(int argc, char *argv[]) {
    config_t cfg = {0};

    int opt;
    while ((opt = getopt(argc, argv, "w:t:s:")) != -1) {
        switch (opt)
        {
        case 'w':
            cfg.workers = atoi(optarg);
            break;
        case 't':
            cfg.tasks = atoi(optarg);
            break;
        case 's':
            cfg.queue_size = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -w <workers> -t <tasks> -s <queue_size>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(cfg.workers <= 0 || cfg.tasks <= 0 || cfg.queue_size <= 0) {
        fprintf(stderr, "All parameters need to be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

void start_worker(int worker_id, const char *queue_name);

int main(int argc, char * argv[]) {
    srand(time(NULL)); //intialise Randomnumbers
    config_t cfg = parse_arguments(argc, argv);

    const char *task_queue_name = "/task_queue";
    const char *result_queue_name = "/result_queue";

    //set the attributes
    struct mq_attr task_attr = {
        .mq_flags = 0,
        .mq_maxmsg = cfg.queue_size,
        .mq_msgsize = sizeof(int),
        .mq_curmsgs = 0
    };

    //delete if queue exist
    mq_unlink(task_queue_name);

    //create queue
    mqd_t task_q = mq_open(queue_name, O_CREAT | O_RDWR, 0655, &attr);
    if (task_q == (mqd_t)-1) {
        perror("mq_open failed!");
        exit(EXIT_FAILURE);
    }

    struct mq_attr result_attr = {
        .mq_flags = 0,
        .mq_maxmsg = cfg.queue_size,
        .mq_msgsize = sizeof(result_t),
        .mq_curmsgs = 0;
    };
    
    mq_unlink(result_queue_name);
    mqd_t result_q = mq_open(result_queue_name, O_CREAT | O_RDONLY, 0644, &result_attr);
    if (result_q == (mqd_t)-1) {
        perror("mq_open (result) failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < cfg.workers; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed!");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            //chilld process ---> worker
            int worker_id = i + 1;
            pid_t child_pid = getpid();

            printf("%02ld:%02ld:%02ld | Worker #%02d | Started worker PID %d\n",
                (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
                worker_id, child_pid);

            //exit(0); // for testing
            start_worker(worker_id, queue_name);
        }
    }

    printf("%02ld:%02ld:%02ld | Ventilator | Distributing tasks\n",
        (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60));
        
    //send the tasks
    for (int i = 0; i < cfg.tasks; i++) {
        int effort = (rand() %10) + 1;

        printf("%02ld:%02ld:%02ld | Ventilator | Queuing task #%d with effort %d\n",
            (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
            i + 1, effort);
 
        if (mq_send(task_q, (char *)&effort, sizeof(int), 0) == -1) {
            perror("mq_send failed");
            exit(EXIT_FAILURE);
        }
    }

    //terminisationsignals to all worker
    printf("%02ld:%02ld:%02ld | Ventilator | Sending termination tasks\n",
        (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60));
 
    int stop_signal = 0;
    for (int i = 0; i < cfg.workers; ++i) {
        if (mq_send(task_q, (char *)&stop_signal, sizeof(int), 0) == -1) {
            perror("mq_send termination failed");
            exit(EXIT_FAILURE);
        }
    }


    printf("Ventilator gestartet mit %d Workern, %d Tasks, Queue Size %d\n",
        cfg.workers, cfg.tasks, cfg.queue_size);


    //close and unlink 
    /*mq_close(task_q);
    mq_unlink(queue_name);
    printf("Queue get closed and deleted\n");*/

    // collet child processes (wait)
    for (int i = 0; i < cfg.workers; ++i) {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0) {
            if (WIFEXITED(status)) {
            printf("Worker with PID %d ended (Exit-Code: %d)\n", pid, WEXITSTATUS(status));
            } else {
            printf(" Worker with PID %d has not be clean ended\n", pid);
            }
        }
    }
    mq_close(task_q);
    mq_unlink(queue_name);
    printf("Queue get closed and deleted\n");

    return 0;
}

void start_worker(int worker_id, const char *queue_name) {
    mqd_t q = mq_open(queue_name, O_RDONLY);
    if (q == (mqd_t)-1) {
        perror("Worker: mq_open failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int effort;
        ssize_t bytes = mq_receive(q, (char *)&effort, sizeof(int), NULL);
        if (bytes == -1) {
            perror("mq_receive failed");
            continue;
        }

        if (effort == 0) {
            printf("%02ld:%02ld:%02ld | Worker #%02d | Received termination task\n",
                   (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
                   worker_id);
            break;
        }

        printf("%02ld:%02ld:%02ld | Worker #%02d | Received task with effort %d\n",
               (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
               worker_id, effort);

        sleep(effort);
    }

    mq_close(q);
    exit(0);
}
