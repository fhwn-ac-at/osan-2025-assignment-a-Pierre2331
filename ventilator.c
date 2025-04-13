#include <mqueue.h>   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

// Configuration struct for command-line arguments
typedef struct {
    int workers;       // number of worker processes
    int tasks;         // number of tasks to process
    int queue_size;    // max number of messages in the queue
} config_t;

// Result struct sent by each worker at the end
typedef struct {
    int worker_id;          // unique worker ID (starts at 1)
    pid_t pid;              // process ID of worker
    int task_count;         // number of tasks processed
    int total_sleep_time;   // total time "worked" (simulated by sleep)
} result_t;

// Function to parse command-line arguments
config_t parse_arguments(int argc, char *argv[]) {
    config_t cfg = {0}; // default all values to 0

    int opt;
    // Read options -w <workers> -t <tasks> -s <queue_size>
    while ((opt = getopt(argc, argv, "w:t:s:")) != -1) {
        switch (opt) {
            case 'w': cfg.workers = atoi(optarg); break;
            case 't': cfg.tasks = atoi(optarg); break;
            case 's': cfg.queue_size = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -w <workers> -t <tasks> -s <queue_size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Check for valid values
    if(cfg.workers <= 0 || cfg.tasks <= 0 || cfg.queue_size <= 0) {
        fprintf(stderr, "All parameters need to be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

// This function runs inside the worker process
void start_worker(int worker_id, const char *task_queue_name, const char *result_queue_name) {
    // Open task queue (read-only)
    mqd_t task_q = mq_open(task_queue_name, O_RDONLY);
    if (task_q == (mqd_t)-1) {
        perror("Worker: mq_open (task) failed");
        exit(EXIT_FAILURE);
    }

    // Open result queue (write-only)
    mqd_t result_q = mq_open(result_queue_name, O_WRONLY);
    if (result_q == (mqd_t)-1) {
        perror("Worker: mq_open (result) failed");
        exit(EXIT_FAILURE);
    }

    int task_count = 0;
    int total_sleep_time = 0;

    // Main loop: wait for tasks
    while (1) {
        int effort;
        // Receive a task
        ssize_t bytes = mq_receive(task_q, (char *)&effort, sizeof(int), NULL);
        if (bytes == -1) {
            perror("Worker: mq_receive failed");
            continue;
        }

        // Check for termination signal
        if (effort == 0) {
            printf("%02ld:%02ld:%02ld | Worker #%02d | Received termination task\n",
                   (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
                   worker_id);
            break;
        }

        // Simulate work
        printf("%02ld:%02ld:%02ld | Worker #%02d | Received task with effort %d\n",
               (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
               worker_id, effort);

        task_count++;
        total_sleep_time += effort;
        sleep(effort); // simulate effort
    }

    // Send results back to the ventilator
    result_t result = {
        .worker_id = worker_id,
        .pid = getpid(),
        .task_count = task_count,
        .total_sleep_time = total_sleep_time
    };

    if (mq_send(result_q, (char *)&result, sizeof(result_t), 0) == -1) {
        perror("Worker: mq_send result failed");
        exit(EXIT_FAILURE);
    }

    // Clean up
    mq_close(task_q);
    mq_close(result_q);
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(NULL)); // seed random number generator
    config_t cfg = parse_arguments(argc, argv); // parse CLI args

    const char *task_queue_name = "/task_queue";
    const char *result_queue_name = "/result_queue";

    // Define attributes for both queues
    struct mq_attr task_attr = {
        .mq_flags = 0,
        .mq_maxmsg = cfg.queue_size,
        .mq_msgsize = sizeof(int),
        .mq_curmsgs = 0
    };

    struct mq_attr result_attr = {
        .mq_flags = 0,
        .mq_maxmsg = cfg.queue_size,
        .mq_msgsize = sizeof(result_t),
        .mq_curmsgs = 0
    };

    // Remove any old queues from previous runs
    mq_unlink(task_queue_name);
    mq_unlink(result_queue_name);

    // Create new task and result queues
    mqd_t task_q = mq_open(task_queue_name, O_CREAT | O_RDWR, 0655, &task_attr);
    if (task_q == (mqd_t)-1) {
        perror("mq_open task failed");
        exit(EXIT_FAILURE);
    }

    mqd_t result_q = mq_open(result_queue_name, O_CREAT | O_RDONLY, 0644, &result_attr);
    if (result_q == (mqd_t)-1) {
        perror("mq_open result failed");
        exit(EXIT_FAILURE);
    }

    // Log: Starting
    printf("%02ld:%02ld:%02ld | Ventilator | Starting %d workers for %d tasks and a queue size of %d\n",
           (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
           cfg.workers, cfg.tasks, cfg.queue_size);

    // Start worker processes
    for (int i = 0; i < cfg.workers; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child process becomes a worker
            int worker_id = i + 1;
            printf("%02ld:%02ld:%02ld | Worker #%02d | Started worker PID %d\n",
                   (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
                   worker_id, getpid());

            start_worker(worker_id, task_queue_name, result_queue_name);
        }
    }

    // Log: Task distribution
    printf("%02ld:%02ld:%02ld | Ventilator | Distributing tasks\n",
           (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60));

    // Generate and send tasks with random "effort"
    for (int i = 0; i < cfg.tasks; ++i) {
        int effort = (rand() % 10) + 1;
        printf("%02ld:%02ld:%02ld | Ventilator | Queuing task #%d with effort %d\n",
               (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
               i + 1, effort);

        if (mq_send(task_q, (char *)&effort, sizeof(int), 0) == -1) {
            perror("mq_send task failed");
            exit(EXIT_FAILURE);
        }
    }

    // Send termination signal to all workers
    printf("%02ld:%02ld:%02ld | Ventilator | Sending termination tasks\n",
           (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60));

    int stop_signal = 0;
    for (int i = 0; i < cfg.workers; ++i) {
        if (mq_send(task_q, (char *)&stop_signal, sizeof(int), 0) == -1) {
            perror("mq_send stop failed");
            exit(EXIT_FAILURE);
        }
    }

    // Collect result messages from each worker
    for (int i = 0; i < cfg.workers; ++i) {
        result_t result;
        ssize_t bytes = mq_receive(result_q, (char *)&result, sizeof(result_t), NULL);
        if (bytes == -1) {
            perror("mq_receive result failed");
            continue;
        }

        printf("%02ld:%02ld:%02ld | Ventilator | Worker %d processed %d tasks in %d seconds\n",
               (long)(time(NULL) / 3600 % 24), (long)(time(NULL) / 60 % 60), (long)(time(NULL) % 60),
               result.worker_id, result.task_count, result.total_sleep_time);
    }

    // Wait for all child processes to terminate
    for (int i = 0; i < cfg.workers; ++i) {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0) {
            if (WIFEXITED(status)) {
                printf("Ventilator | Worker with PID %d exited with status %d\n", pid, WEXITSTATUS(status));
            } else {
                printf("Ventilator | Worker with PID %d did not exit cleanly\n", pid);
            }
        }
    }

    // Clean up resources
    mq_close(task_q);
    mq_close(result_q);
    mq_unlink(task_queue_name);
    mq_unlink(result_queue_name);

    printf("Ventilator | All workers done. Resources cleaned up.\n");
    return 0;
}
