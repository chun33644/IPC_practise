
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>


static int pipefd[2];
static bool new_result = false;

static bool end_of_thread = false;

static pthread_t task1_id;

/*
typedef struct _task_info{
    pthread_t task_id;
    time_t start_time;
    char name[32];
    bool create_status;    //enum....
} task_info;
*/



static void signal_handler(int sig) {

    char cmd = 0;
    if (sig == SIGUSR1) {
        cmd = 'C';
    } else if (sig == SIGUSR2) {
        cmd = 'D';
    } else {
        return;
    }

    if (cmd) {
        write(pipefd[1], &cmd, 1);
        //write(2, "for test", 8);
    }

}


void *create_task_handler(void *arg) {
    printf("create thread success, PID : %d\n", getpid());

    while (!end_of_thread) {
        printf("Hello I am thread\n");
        sleep(5);
    }

    return NULL;
}


int main() {

    //create pipe
    int pi_res = pipe(pipefd);
    if (pi_res != 0) {
        perror("pipe error:");
        return -1;
    }

    //create two signal to trigger the event
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;

    int sig1_res = sigaction(SIGUSR1, &sa, NULL);
    if (sig1_res != 0) {
        perror("sig1 error:");
        return -2;
    }

    int sig2_res = sigaction(SIGUSR2, &sa, NULL);
    if (sig2_res != 0) {
        perror("sig1 error:");
        return -3;
    }

    printf("PID = %d\n", getpid());
    printf("kill -USR1 %d #create task\n", getpid());
    printf("kill -USR2 %d #delete tasl\n", getpid());

    char cmd = 0;
    while (1) {
        ssize_t re_res = read(pipefd[0], &cmd, 1);
        if (re_res <= 0) {
            perror("read wait......");
            continue;
        }

        if (cmd == 'C') {
            printf("test for enter pthread_create.....\n");
            int new_res = pthread_create(&task1_id, NULL, create_task_handler, NULL);
            if (new_res != 0){
                perror("create thread fail:");
                return -4;
            } else {
                new_result = true;
            }
        }

        if (cmd == 'D') {
            if (!new_result) {
                return -5;
            } else {
                // do something kill thread
                end_of_thread = true;
                // waiting thtread finish
                printf("ready to join\n");
                int de_res = pthread_join(task1_id, NULL);
                printf("join finish\n");
                if (de_res != 0) {
                    perror("delete thread fail:");
                    return -6;
                }
                printf("delete thread success");
                return 0;
            }
        }
    }

    return 0;
}







