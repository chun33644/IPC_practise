#ifndef MQ_SENDER_H
#define MQ_SENDER_H


#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>


#define MSGQ_LINK               "/Chun_MQ"
#define MSGQ_MAX_COUNT          10
#define MSGQ_MAX_MSGSIZE        100


typedef struct _message_package {
    char buff[MSGQ_MAX_MSGSIZE];
    int actual_len;
} message_package;

typedef struct messageQueue_struct {
    mqd_t qd;
    struct mq_attr attr;
    message_package pkg;
} messageQueue_struct;





void update_mqattr_content(struct messageQueue_struct *info);



#endif
