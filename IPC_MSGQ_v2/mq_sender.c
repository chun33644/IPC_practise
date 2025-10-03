#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>

#include "mq_info.h"



static messageQueue_struct send_info = {
    .qd = 0,
    .attr.mq_flags = 0,     //not use O_NONBLOCK
    .attr.mq_maxmsg = MSGQ_MAX_COUNT,
    .attr.mq_msgsize = sizeof(message_package),
    .attr.mq_curmsgs = 0,
    .pkg.buff = {0},
    .pkg.actual_len = 0
};


int msg_queue_init(const char *link, struct messageQueue_struct *info) {

    mq_unlink(link);
    info->qd = mq_open(link, O_CREAT | O_EXCL | O_WRONLY, 0600, &info->attr);
    if (info->qd < 0) {
        perror("mq_open");
    }

    return 0;
}




int mqSend_task(struct messageQueue_struct *info) {

    update_mqattr_content(info);
    unsigned int prio = 40;
/*
    int count = 0;
    while (1) {

        //sleep(1);
        snprintf(info->pkg.buff, 100, "Hello..(%d)", count);
        int s_res = mq_send(info->qd, (const char *)&info->pkg, sizeof(message_package), prio);
        if (s_res < 0) {
            perror("mq_send");
            mq_close(info->qd);
            return -1;
        }
        printf("Send: %s, Size: %ld\n", info->pkg.buff, info->attr.mq_msgsize);
        count ++;
    }
*/
    for (int idx = 0; idx < MSGQ_MAX_COUNT; idx++) {

        snprintf(info->pkg.buff, sizeof(info->pkg.buff), "Hello..(%d)", idx);
        info->pkg.actual_len = strlen((const char *)info->pkg.buff);
        if (info->pkg.actual_len > MSGQ_MAX_MSGSIZE) {
            printf("len > mq_msgsize\n");
            return -1;
        }

        int s_res = mq_send(info->qd, (const char *)&info->pkg, sizeof(message_package), prio);
        if (s_res < 0) {
            perror("mq_send");
            mq_close(info->qd);
            unlink(MSGQ_LINK);
            return -2;
        }
        printf("Send: %s, Len: %d\n", info->pkg.buff, info->pkg.actual_len);
    }


    printf("Input some text or enter 'end' to finish\n");
    while (1) {

        fgets(info->pkg.buff, MSGQ_MAX_MSGSIZE, stdin);
        info->pkg.actual_len = strlen((const char *)info->pkg.buff);
        int res = mq_send(info->qd, (const char *)&info->pkg, sizeof(message_package), prio);
        if (res < 0) {
            perror("mq_send(manual input text)");
            mq_close(info->qd);
            unlink(MSGQ_LINK);
            return -3;
        }

        //printf("for test buff: %s, len: %d\n", info->pkg.buff, info->pkg.actual_len);
        if (strncmp("end", (const char *)info->pkg.buff, 3) == 0) {
            break;
        }

    }


    return 0;
}


int main() {

    msg_queue_init(MSGQ_LINK, &send_info);

    int run = mqSend_task(&send_info);
    if (run < 0) {
        printf("mqSend_task running error");
        mq_unlink(MSGQ_LINK);
    }

    mq_close(send_info.qd);


    return 0;
}
