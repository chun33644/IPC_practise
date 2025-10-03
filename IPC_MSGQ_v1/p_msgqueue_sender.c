
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>

mqd_t mqd;
struct mq_attr attr;


int main() {


    mq_unlink("/OpenCSF_MQ");

    //mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr)
    //(mode_t)0600 means that the file will have read and write permissions
    mqd = mq_open("/OpenCSF_MQ", O_CREAT | O_EXCL | O_WRONLY, 0600, NULL);
    if (mqd < 0) {
        perror("mq_open");
    }

    //int mq_send(mqd_t mqdes, const char msg_ptr[msg_len], size_t msg_len, unsigned int msg_prio)
    //mq_send(mqd, "HELLO", 8, 10);
    //mq_send(mqd, "HELLO", 8, 20);
    //mq_send(mqd, "HELLO", 8, 30);
    //mq_send(mqd, "HELLO", 8, 40);

    char msg[100] = {0};

    int count = 0;
    while (1) {

/*
        printf("Input some text :\n");
        fgets(msg, 100, stdin);
        printf("enter: %s\n", msg);
        mq_send(mqd, msg, 100, 50);
*/
        snprintf(msg, 100, "Hello...(%d)", count);
        mq_send(mqd, msg, 100, 40);
        count ++;

        //sleep(1);
    }

    mq_close(mqd);


    return 0;
}
