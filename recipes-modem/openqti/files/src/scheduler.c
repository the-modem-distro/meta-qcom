// SPDX-License-Identifier: MIT
#include "../inc/scheduler.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include "../inc/sms.h"
#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct {
  bool in_use;
  struct timespec startup_time;
  struct timespec prev_tick;
  struct timespec curr_tick;
  struct task_p tasks[MAX_NUM_TASKS];
} sch_runtime;

/*
 *
 * Simple scheduler to keep track of things
 *
 *
 */

int find_free_task_slot() {
  for (int i = 0; i < MAX_NUM_TASKS; i++) {
    if (sch_runtime.tasks[i].status == 0) {
      return i;
    }
  }
  return -ENOSPC;
}

int add_task(struct task_p *task) {
  int taskID;
  logger(MSG_INFO, "%s: Adding task: Type: %i, param: %i, arg: %s", __func__,
         task->type, task->param, task->arguments);
  taskID = find_free_task_slot();
  if (taskID < 0) {
    logger(MSG_ERROR, "%s: No available slots, task not added\n", __func__);
    return -ENOSPC;
  } else {
    logger(MSG_INFO, "%s: Adding task %i to the queue\n", __func__, taskID);
    sch_runtime.tasks[taskID].type = task->type;
    sch_runtime.tasks[taskID].param = task->param;
    memcpy(sch_runtime.tasks[taskID].arguments, task->arguments, ARG_SIZE);
    sch_runtime.tasks[taskID].status = STATUS_PENDING;
  }
  return taskID;
}

int remove_task(int taskID) {
  if (taskID < MAX_NUM_TASKS) {
    sch_runtime.tasks[taskID].status = 0;
    sch_runtime.tasks[taskID].param = 0;
    sch_runtime.tasks[taskID].type = 0;
    memset(sch_runtime.tasks[taskID].arguments, 0, ARG_SIZE);
    return 0;
  }
  return -EINVAL;
}

int update_task(int taskID, uint8_t status) {
  if (taskID < MAX_NUM_TASKS) {
    sch_runtime.tasks[taskID].status = status;
    return 0;
  }

  return -EINVAL;
}

void cleanup_tasks() {
  int ret;
  for (int i = 0; i < MAX_NUM_TASKS; i++) {
    if (sch_runtime.tasks[i].status == STATUS_DONE ||
        sch_runtime.tasks[i].status == STATUS_FAILED) {
      logger(MSG_INFO, "%s: Removing task %i with status %i\n", __func__, i,
             sch_runtime.tasks[i].status);
      ret = remove_task(i);
    }
  }
}

int run_task(int taskID) {
  logger(MSG_INFO, "%s: Running task %i\n", __func__, taskID);
  if (taskID < 0 || taskID > MAX_NUM_TASKS) {
    logger(MSG_ERROR, "%s: Invalid task\n", __func__);
    return -EINVAL;
  }

  if (sch_runtime.tasks[taskID].status == STATUS_FREE ||
      sch_runtime.tasks[taskID].status == STATUS_DONE ||
      sch_runtime.tasks[taskID].status == STATUS_FAILED) {
    logger(MSG_WARN, "%s: Attempted to run a task with status %i", __func__,
           sch_runtime.tasks[taskID].status);
    return -EINVAL;
  }
  sch_runtime.tasks[taskID].status = STATUS_IN_PROGRESS;
  switch (sch_runtime.tasks[taskID].type) {
  case TASK_TYPE_SMS:
    logger(MSG_INFO, "%s: Send SMS \n", __func__);
    break;
  case TASK_TYPE_CALL:
    logger(MSG_INFO, "%s: Call admin\n", __func__);
    break;
  case TASK_TYPE_WAKE_HOST:
    logger(MSG_INFO, "%s: Try to wake up the host\n", __func__);
    break;
  }
  return 0;
}

void *start_scheduler_thread() {
  int i, ret;
  logger(MSG_INFO, "%s: Starting scheduler thread\n", __func__);
  while (1) {
    for (i = 0; i < MAX_NUM_TASKS; i++) {
      if (sch_runtime.tasks[i].status == STATUS_PENDING) {
        logger(MSG_INFO, "%s: Running task %i of type %i\n", __func__, i,
               sch_runtime.tasks[i].type);
        ret = run_task(i);
      }
    }
    logger(MSG_INFO, "%s Tick!\n", __func__);
    sleep(1);
  }
  return NULL;
}