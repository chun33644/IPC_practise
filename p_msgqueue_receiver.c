

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MSGQ_LINK   "/OpenCSF_MQ"

/*
    for test :
        as long as the child_process gets the msg,
        the parent_process will stop and end the program
*/



pid_t pid;
static volatile sig_atomic_t end_flag = 1;


/* sigation handler */
void sign_stop() {

    end_flag = 0;
}



void parent_process(mqd_t *new_mqd, struct mq_attr *new_attr) {


    *new_mqd = mq_open(MSGQ_LINK, O_RDONLY);
    if (*new_mqd < 0) {
        perror("mq_open");
    }

    int get_res = mq_getattr(*new_mqd, new_attr);
    if (get_res < 0) {
        perror("mq_getattr");
        end_flag = 0;
    }

    char *buff = malloc(sizeof(new_attr->mq_msgsize));
    if (buff == NULL) {
        perror("malloc failed");
    }


    unsigned int priority = 0;

    while (end_flag) {

        ssize_t rec_bytes = mq_receive(*new_mqd, buff, new_attr->mq_msgsize, &priority);
        if (rec_bytes == -1) {
            printf("(%s) failed to receive message\n", __func__);
            continue;
        } else if (rec_bytes > 0) {
            printf("(%s) Received [priority %u]: '%s'\n", __func__, priority, buff);
        }

        memset(buff, 0, sizeof(new_attr->mq_msgsize));
    }

    free(buff);
    buff = NULL;

    printf("exit (%s)\n", __func__);

    close(*new_mqd);
    unlink(MSGQ_LINK);
}


/*

struct mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
};

*/


/* once the child_process is entered, end_flag will be activated to close the parent_process */
void child_process(mqd_t *new_mqd, struct mq_attr *new_attr) {

    *new_mqd = mq_open(MSGQ_LINK, O_RDONLY);
    if (*new_mqd < 0) {
        perror("mq_open");
    }

    int get_res = mq_getattr(*new_mqd, new_attr);
    if (get_res < 0) {
        perror("mq_getattr");
        end_flag = 0;
    }

    char *buff = malloc(sizeof(new_attr->mq_msgsize));
    if (buff == NULL) {
        perror("malloc failed");
    }


    unsigned int priority = 0;
    ssize_t rec_bytes = mq_receive(*new_mqd, buff, new_attr->mq_msgsize, &priority);
    if (rec_bytes == -1) {
        printf("(%s) failed to receive message\n", __func__);
    } else if (rec_bytes > 0) {
        printf("(%s) Received [priority %u]: '%s'\n", __func__, priority, buff);
    }

    free(buff);
    buff = NULL;

    close(*new_mqd);

    kill(getppid(), SIGUSR1);

    printf("exit (%s)\n", __func__);
    exit(EXIT_SUCCESS);
}

int main() {


    //register a signal 'SIGUSR1' to kill() stop parent_process
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sign_stop;
    sa.sa_flags = 0;

    int sig1_res = sigaction(SIGUSR1, &sa, NULL);
    if (sig1_res != 0) {
        perror("sig1 error:");
    }


    mqd_t mqd_p;
    mqd_t mqd_c;

    struct mq_attr attr_p;
    struct mq_attr attr_c;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    //use fork() to identify the parent process or child process
    if (pid > 0) {
        printf("enter the parent process, ID: %d\n", pid);
        parent_process(&mqd_p, &attr_p);
    } else {
        printf("enter the child process, ID: %d\n", pid);
        child_process(&mqd_c, &attr_c);
    }


    printf("exit (%s)\n", __func__);
    return 0;
}
