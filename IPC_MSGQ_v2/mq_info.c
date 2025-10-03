

#include "mq_info.h"


void update_mqattr_content(struct messageQueue_struct *info) {

    int res = mq_getattr(info->qd, &info->attr);
    if (res < 0) {
        perror("mq_getattr");
    }

}
