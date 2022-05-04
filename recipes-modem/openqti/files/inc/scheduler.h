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

/* Time mode */
enum { SCHED_MODE_TIME_AT = 0, SCHED_MODE_TIME_COUNTDOWN };
/*
 *  Status:
 *      0 -> free
 *      1 -> Pending
 *      2 -> In progress
 *      3 -> Done
 */

struct execution_time {
  time_t exec_time;
  uint8_t mode; // 0 -> Specific time ; 1 -> In specific time from creation
  uint8_t hh;
  uint8_t mm;
};

struct task_p {
  uint8_t type;
  uint8_t param;
  uint8_t status;
  struct execution_time time;
  char arguments[ARG_SIZE];
};

void *start_scheduler_thread();
int add_task(struct task_p task);
void dump_pending_tasks();
int remove_task(int taskID);
#endif