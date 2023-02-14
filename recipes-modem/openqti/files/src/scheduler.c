// SPDX-License-Identifier: MIT
#include "../inc/scheduler.h"
#include "../inc/call.h"
#include "../inc/config.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include "../inc/nas.h"
#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include "../inc/sms.h"
#include "../inc/space_mon.h"

#include <endian.h>
#include <errno.h>
#include <stdio.h>
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

int save_tasks_to_storage() {
  FILE *fp;
  int ret;
  logger(MSG_DEBUG, "%s: Start\n", __func__);
  if (set_persistent_partition_rw() < 0) {
    logger(MSG_ERROR, "%s: Can't set persist partition in RW mode\n", __func__);
    return -1;
  }
  logger(MSG_DEBUG, "%s: Open file\n", __func__);
  fp = fopen(SCHEDULER_DATA_FILE_PATH, "w");
  if (fp == NULL) {
    logger(MSG_ERROR, "%s: Can't open config file for writing\n", __func__);
    return -1;
  }
  logger(MSG_DEBUG, "%s: Store\n", __func__);
  ret = fwrite(sch_runtime.tasks, sizeof(struct task_p), MAX_NUM_TASKS, fp);
  logger(MSG_DEBUG, "%s: Close (%i bytes written)\n", __func__, ret);
  fclose(fp);
  do_sync_fs();
  if (!use_persistent_logging()) {
    if (set_persistent_partition_ro() < 0) {
      logger(MSG_ERROR, "%s: Can't set persist partition in RO mode\n",
             __func__);
      return -1;
    }
  }
  return 0;
}

int read_tasks_from_storage() {
  FILE *fp;
  int ret;
  struct task_p tasks[MAX_NUM_TASKS];
  logger(MSG_DEBUG, "%s: Start, open file\n", __func__);
  fp = fopen(SCHEDULER_DATA_FILE_PATH, "r");
  if (fp == NULL) {
    logger(MSG_DEBUG, "%s: Can't open config file for reading\n", __func__);
    return -1;
  }
  ret = fread(tasks, sizeof(struct task_p), MAX_NUM_TASKS, fp);
  logger(MSG_DEBUG, "%s: Close (%i bytes read)\n", __func__, ret);
  if (ret >= sizeof(struct task_p)) {
    logger(MSG_DEBUG, "%s: Recovering tasks\n ", __func__);
    for (int i = 0; i < MAX_NUM_TASKS; i++) {
      sch_runtime.tasks[i] = tasks[i];
    }
  }
  fclose(fp);
  return 0;
}

void delay_task_execution(int taskID, uint8_t seconds) {
  sch_runtime.tasks[taskID].time.exec_time += seconds;
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
        new_time.tm_mday = tm.tm_mday + 1;
      }
      new_time.tm_hour = sch_runtime.tasks[taskID].time.hh;
      new_time.tm_min = sch_runtime.tasks[taskID].time.mm;
      sch_runtime.tasks[taskID].time.exec_time = mktime(&new_time);
      logger(MSG_INFO, "Current time: %i-%i-%i %i:%i:%i\n", tm.tm_year,
             tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      logger(MSG_INFO, "Target time: %i-%i-%i %i:%i:%i\n", new_time.tm_year,
             new_time.tm_mon, new_time.tm_mday, new_time.tm_hour,
             new_time.tm_min, new_time.tm_sec);
      logger(MSG_INFO, "From echo: Now %ld -> Exec at %ld\n", t,
             sch_runtime.tasks[taskID].time.exec_time);
      break;
    case SCHED_MODE_TIME_COUNTDOWN:
      sch_runtime.tasks[taskID].time.exec_time =
          t + sch_runtime.tasks[taskID].time.hh * 3600 +
          sch_runtime.tasks[taskID].time.mm * 60;

      break;
    }
  }
  save_tasks_to_storage();
  return taskID;
}

int remove_task(int taskID) {
  int ret = -EINVAL;
  if (taskID < MAX_NUM_TASKS) {
    if (sch_runtime.tasks[taskID].status != STATUS_FREE) {
      ret = 0;
    }
    sch_runtime.tasks[taskID].status = 0;
    sch_runtime.tasks[taskID].param = 0;
    sch_runtime.tasks[taskID].type = 0;
    sch_runtime.tasks[taskID].time.hh = 0;
    sch_runtime.tasks[taskID].time.mm = 0;
    sch_runtime.tasks[taskID].time.mode = 0;
    memset(sch_runtime.tasks[taskID].arguments, 0, ARG_SIZE);
    save_tasks_to_storage();
  }
  return ret;
}

int remove_all_tasks_by_type(uint8_t task_type) {
  for (int i = 0; i < MAX_NUM_TASKS; i++) {
    if (sch_runtime.tasks[i].type == task_type) {
      remove_task(i);
    }
  }
  return 0;
}
int update_task_status(int taskID, uint8_t status) {
  if (taskID < MAX_NUM_TASKS) {
    sch_runtime.tasks[taskID].status = status;
    return 0;
  }

  return -EINVAL;
}

void cleanup_tasks() {
  bool needs_write = false;
  for (int i = 0; i < MAX_NUM_TASKS; i++) {
    if (sch_runtime.tasks[i].status == STATUS_DONE ||
        sch_runtime.tasks[i].status == STATUS_FAILED) {
      logger(MSG_INFO, "%s: Removing task %i with status %i\n", __func__, i,
             sch_runtime.tasks[i].status);
      remove_task(i);
      needs_write = true;
    }
  }
  if (needs_write)
    save_tasks_to_storage();
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
    sch_runtime.tasks[taskID].status = STATUS_DONE;
    break;
  case TASK_TYPE_CALL:
    logger(MSG_INFO, "%s: Call admin\n", __func__);
    if (get_call_simulation_mode()) {
      delay_task_execution(taskID, 60);
    } else {
      set_pending_call_flag(true);
      set_looped_message(true);
      add_voice_message_to_queue((uint8_t *)sch_runtime.tasks[taskID].arguments,
                                 strlen(sch_runtime.tasks[taskID].arguments));
      sch_runtime.tasks[taskID].status = STATUS_DONE;
    }
    break;
  case TASK_TYPE_WAKE_HOST:
    logger(MSG_INFO, "%s: Try to wake up the host\n", __func__);
    pulse_ring_in();
    sch_runtime.tasks[taskID].status = STATUS_DONE;
    break;
  case TASK_TYPE_DND_CLEAR:
    logger(MSG_INFO, "%s: Clear do not disturb mode\n", __func__);
    set_do_not_disturb(false);
    sch_runtime.tasks[taskID].status = STATUS_DONE;
    break;
  }

  cleanup_tasks();
  return 0;
}

void *start_scheduler_thread() {
  int tick = 0;
  do {
    sleep(5);
    logger(MSG_INFO, "%s: Waiting for network...\n", __func__);
  } while (!nas_is_network_in_service());
  /*
    We should check here if time is synced by now...
  */
  logger(MSG_INFO, "%s: Starting scheduler thread\n", __func__);
  read_tasks_from_storage();
  while (1) {
    sch_runtime.cur_time = time(NULL);
    for (int i = 0; i < MAX_NUM_TASKS; i++) {
      if (sch_runtime.tasks[i].status == STATUS_PENDING) {
        logger(MSG_DEBUG,
               "%s: Checking time for task %i of type %i\n Exec time %ld, "
               "current %ld\n",
               __func__, i, sch_runtime.tasks[i].type,
               sch_runtime.tasks[i].time.exec_time, sch_runtime.cur_time);
        if (sch_runtime.cur_time >= sch_runtime.tasks[i].time.exec_time) {
          run_task(i);
        }
      }
    }
    logger(MSG_DEBUG, "%s Tick!\n", __func__);
    sleep(1);
    if (tick > 10) {
      logger(MSG_DEBUG, "%s: Tock!\n", __func__);
      tick = 0;
      // Request network update
      nas_request_signal_info();
      nas_request_cell_location_info();
      uint32_t avail_space_persist = get_available_space_persist_mb();
      uint32_t avail_space_tmpfs = get_available_space_tmpfs_mb();
      if (avail_space_persist != -EINVAL && avail_space_persist < 1) {
        uint8_t reply[MAX_MESSAGE_SIZE] = {0};
        size_t strsz =
            snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "I'm running out of storage, cleaning /persist\n");
        add_message_to_queue(reply, strsz);
        cleanup_storage(1, false, "openqti.lock");
      }
      if (avail_space_tmpfs != -EINVAL && avail_space_tmpfs < 1) {
        uint8_t reply[MAX_MESSAGE_SIZE] = {0};
        size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                                "I'm running out of memory, cleaning /tmp\n");
        add_message_to_queue(reply, strsz);
        cleanup_storage(0, true, "openqti.lock");
      }
    }
    tick++;
  }
  return NULL;
}

void dump_pending_tasks() {
  int strsz = 0;
  int count = 0;
  char reply[MAX_MESSAGE_SIZE];
  for (int i = 0; i < MAX_NUM_TASKS; i++) {
    if (sch_runtime.tasks[i].status == STATUS_PENDING) {
      count++;
      memset(reply, 0, MAX_MESSAGE_SIZE);
      switch (sch_runtime.tasks[i].type) {
      case TASK_TYPE_CALL:
        strsz = snprintf(
            reply, MAX_MESSAGE_SIZE,
            "Task %i:\n Call me (%ld min to exec): args: %s", i,
            (sch_runtime.tasks[i].time.exec_time - sch_runtime.cur_time) / 60,
            sch_runtime.tasks[i].arguments);
        break;
      case TASK_TYPE_SMS:
        strsz = snprintf(
            reply, MAX_MESSAGE_SIZE,
            "Task %i:\n Text me (%ld min to exec): args: %s", i,
            (sch_runtime.tasks[i].time.exec_time - sch_runtime.cur_time) / 60,
            sch_runtime.tasks[i].arguments);
        break;
      case TASK_TYPE_WAKE_HOST:
        strsz = snprintf(
            reply, MAX_MESSAGE_SIZE,
            "Task %i:\n Wake host up (%ld min to exec)", i,
            (sch_runtime.tasks[i].time.exec_time - sch_runtime.cur_time) / 60);
        break;
      }
      add_message_to_queue((uint8_t *)reply, strsz);
    }
  }
  if (count == 0) {
    strsz = snprintf(reply, MAX_MESSAGE_SIZE, "There are no pending tasks!");
    add_message_to_queue((uint8_t *)reply, strsz);
  }
}
