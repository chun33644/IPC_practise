#include <asm-generic/errno.h>
#include <bits/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include <pthread.h>

#include "mq_info.h"

int start = 1;

void* mqReceive_task(void *arg) {

    messageQueue_struct *info = (messageQueue_struct *)arg;
    info->qd = mq_open(MSGQ_LINK, O_RDONLY);
    if (info->qd < 0) {
        perror("mq_open");
    }

/*
    update_mqattr_content(info);
    printf("Attr Info:\n");
    printf("flag: %ld, maxmsg: %ld, msgsize: %ld, curmsg: %ld\n",
           info->attr.mq_flags, info->attr.mq_maxmsg, info->attr.mq_msgsize, info->attr.mq_curmsgs);
*/
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;

    unsigned int prio = 0;


    printf("------------Start mqReceive_task-------------\n");
    while (start) {
        //printf("waiting for sender send msg.....\n");
        ssize_t rec_bytes = mq_timedreceive(info->qd, (char *)&info->pkg, sizeof(message_package), &prio, &ts);
        if (rec_bytes < 0) {
            //perror("mq_receive");
            if (errno == ETIMEDOUT) {
                continue;
            }
        }

        info->pkg.actual_len = strlen((const char *)info->pkg.buff);
        sleep(2);
        printf("QD[%d] Received: %s, Len: %d\n", info->qd, info->pkg.buff, info->pkg.actual_len);

        if (strncmp("end", (const char *)info->pkg.buff, 3) == 0) {
            mq_close(info->qd);
            start = 0;
            break;
        }

    }
   return 0;
}



int main() {


    messageQueue_struct task1_info = {0};
    messageQueue_struct task2_info = {0};
    pthread_t task1, task2;

    int res_1 = pthread_create(&task1, NULL, mqReceive_task, (messageQueue_struct *)&task1_info);
    if (res_1 < 0) {
        perror("pthread_create_1");
    }

    int res_2 = pthread_create(&task2, NULL, mqReceive_task, (messageQueue_struct *)&task2_info);
    if (res_2 < 0) {
        perror("pthread_create_2");
    }


    pthread_join(task1, NULL);
    pthread_join(task2, NULL);
    mq_unlink(MSGQ_LINK);


    return 0;
}

