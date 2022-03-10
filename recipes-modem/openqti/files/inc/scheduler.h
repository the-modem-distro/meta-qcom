/* SPDX-License-Identifier: MIT */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_NUM_TASKS 255
#define ARG_SIZE 160

enum {
    STATUS_FREE = 0,
    STATUS_PENDING,
    STATUS_IN_PROGRESS,
    STATUS_DONE,
    STATUS_FAILED,
};

enum {
    TASK_TYPE_SMS = 0,
    TASK_TYPE_CALL,
    TASK_TYPE_WAKE_HOST,
    /* ....*/
};
/*
 *  Status:
 *      0 -> free
 *      1 -> Pending
 *      2 -> In progress
 *      3 -> Done
 *
 *
 *
 *
 *
 *
 *
 */
struct task_p {
    uint8_t type;
    uint8_t param;
    uint8_t status;
    char arguments[ARG_SIZE];
};


void *start_scheduler_thread();
#endif