// SPDX-License-Identifier: MIT
#include "../inc/scheduler.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include "../inc/sms.h"
#include "../inc/helpers.h"
#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct {
  bool in_use;
  time_t cur_time;
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

void delay_task_execution(int taskID, uint8_t seconds) {
  sch_runtime.tasks[taskID].time.exec_time+= seconds;
}

int add_task(struct task_p task) {
  int taskID;
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  struct tm new_time;
  logger(MSG_INFO, "%s: Adding task: Type: %i, param: %i, arg: %s", __func__,
         task.type, task.param, task.arguments);
  taskID = find_free_task_slot();
  if (taskID < 0) {
    logger(MSG_ERROR, "%s: No available slots, task not added\n", __func__);
    return -ENOSPC;
  } else {
    logger(MSG_INFO, "%s: Adding task %i to the queue\n", __func__, taskID);
    sch_runtime.tasks[taskID] = task;
    sch_runtime.tasks[taskID].status = STATUS_PENDING;
    /* Calculate tick time from now */
    switch (task.time.mode) {
      case SCHED_MODE_TIME_AT:
      new_time = tm;
      new_time.tm_sec = 0;
        if (tm.tm_hour > sch_runtime.tasks[taskID].time.hh) {
          // Change to next day, then let mktime fix the rest
          new_time.tm_mday = tm.tm_mday+1;
        }
        new_time.tm_hour = sch_runtime.tasks[taskID].time.hh;
        new_time.tm_min = sch_runtime.tasks[taskID].time.mm;
        sch_runtime.tasks[taskID].time.exec_time = mktime(&new_time);
        logger(MSG_INFO, "Current time: %i-%i-%i %i:%i:%i\n", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        logger(MSG_INFO, "Target time: %i-%i-%i %i:%i:%i\n", new_time.tm_year, new_time.tm_mon, new_time.tm_mday, new_time.tm_hour, new_time.tm_min, new_time.tm_sec);
        logger(MSG_INFO, "From echo: Now %ld -> Exec at %ld\n", t, sch_runtime.tasks[taskID].time.exec_time);
        break;
      case SCHED_MODE_TIME_COUNTDOWN:
           sch_runtime.tasks[taskID].time.exec_time = t + 
                                                      sch_runtime.tasks[taskID].time.hh * 3600 +
                                                      sch_runtime.tasks[taskID].time.mm * 60;

        break;
    }

  }
  return taskID;
}

int remove_task(int taskID) {
  if (taskID < MAX_NUM_TASKS) {
    sch_runtime.tasks[taskID].status = 0;
    sch_runtime.tasks[taskID].param = 0;
    sch_runtime.tasks[taskID].type = 0;
    sch_runtime.tasks[taskID].time.hh = 0;
    sch_runtime.tasks[taskID].time.mm = 0;
    sch_runtime.tasks[taskID].time.mode = 0;
    memset(sch_runtime.tasks[taskID].arguments, 0, ARG_SIZE);
    return 0;
  }
  return -EINVAL;
}

int update_task_status(int taskID, uint8_t status) {
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
    if (get_call_simulation_mode()) {
      delay_task_execution(taskID, 60);
    } else {
      set_pending_call_flag(true);
      set_looped_message(true);
      add_voice_message_to_queue((uint8_t *)sch_runtime.tasks[taskID].arguments, strlen(sch_runtime.tasks[taskID].arguments));
    }
    break;
  case TASK_TYPE_WAKE_HOST:
    logger(MSG_INFO, "%s: Try to wake up the host\n", __func__);
    pulse_ring_in();
    break;
  }
  return 0;
}

void *start_scheduler_thread() {
  int i, ret;
  logger(MSG_INFO, "%s: Starting scheduler thread\n", __func__);
  while (1) {
    sch_runtime.cur_time = time(NULL);
    for (i = 0; i < MAX_NUM_TASKS; i++) {
      if (sch_runtime.tasks[i].status == STATUS_PENDING) {
        logger(MSG_INFO, "%s: Checking time for task %i of type %i\n Exec time %ld, current %ld\n", __func__, i,
               sch_runtime.tasks[i].type, sch_runtime.tasks[i].time.exec_time, sch_runtime.cur_time);
        if (sch_runtime.cur_time >= sch_runtime.tasks[i].time.exec_time) {
          ret = run_task(i);
        }
      }
    }
    logger(MSG_DEBUG, "%s Tick!\n", __func__);
    sleep(1);
  }
  return NULL;
}