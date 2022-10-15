// SPDX-License-Identifier: MIT

#include "../inc/command.h"
#include "../inc/adspfw.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/cell_broadcast.h"
#include "../inc/config.h"
#include "../inc/logger.h"
#include "../inc/proxy.h"
#include "../inc/scheduler.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct {
  bool is_unlocked;
  uint32_t unlock_time;
  uint8_t cmd_history[1024];
  uint16_t cmd_position;
  uint32_t last_cmd_timestamp;
  char user_name[32];
  char bot_name[32];
} cmd_runtime;

char *get_rt_modem_name() { return cmd_runtime.bot_name; }

char *get_rt_user_name() { return cmd_runtime.user_name; }

void add_to_history(uint8_t command_id) {
  if (cmd_runtime.cmd_position >= 1023) {
    cmd_runtime.cmd_position = 0;
  }
  cmd_runtime.cmd_history[cmd_runtime.cmd_position] = command_id;
  cmd_runtime.cmd_position++;
}

uint16_t find_cmd_history_match(uint8_t command_id) {
  uint16_t match = 0;
  uint16_t countdown;
  if (cmd_runtime.cmd_position > 5) {
    countdown = cmd_runtime.cmd_position - 5;
    for (uint16_t i = cmd_runtime.cmd_position; i >= countdown; i--) {
      if (cmd_runtime.cmd_history[i] == command_id) {
        match++;
      }
    }
  }
  return match;
}

void get_names() {
  get_modem_name(cmd_runtime.bot_name);
  get_user_name(cmd_runtime.user_name);
}
void set_cmd_runtime_defaults() {
  cmd_runtime.is_unlocked = false;
  cmd_runtime.unlock_time = 0;
  strncpy(cmd_runtime.user_name, "User",
          32); // FIXME: Allow user to set a custom name
  strncpy(cmd_runtime.bot_name, "Modem",
          32); // FIXME: Allow to change modem name
  get_names();
}

int get_uptime(uint8_t *output) {
  unsigned updays, uphours, upminutes;
  struct sysinfo info;
  struct tm *current_time;
  time_t current_secs;
  int bytes_written = 0;
  time(&current_secs);
  current_time = localtime(&current_secs);

  sysinfo(&info);

  bytes_written = snprintf((char *)output, MAX_MESSAGE_SIZE,
                           "%02u:%02u:%02u up ", current_time->tm_hour,
                           current_time->tm_min, current_time->tm_sec);
  updays = (unsigned)info.uptime / (unsigned)(60 * 60 * 24);
  if (updays)
    bytes_written += snprintf((char *)output + bytes_written,
                              MAX_MESSAGE_SIZE - bytes_written, "%u day%s, ",
                              updays, (updays != 1) ? "s" : "");
  upminutes = (unsigned)info.uptime / (unsigned)60;
  uphours = (upminutes / (unsigned)60) % (unsigned)24;
  upminutes %= 60;
  if (uphours)
    bytes_written += snprintf((char *)output + bytes_written,
                              MAX_MESSAGE_SIZE - bytes_written, "%2u:%02u",
                              uphours, upminutes);
  else
    bytes_written +=
        snprintf((char *)output + bytes_written,
                 MAX_MESSAGE_SIZE - bytes_written, "%u min", upminutes);

  return 0;
}

int get_load_avg(uint8_t *output) {
  int fd;
  fd = open("/proc/loadavg", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open load average \n", __func__);
    return 0;
  }
  lseek(fd, 0, SEEK_SET);
  if (read(fd, output, 64) <= 0) {
    logger(MSG_ERROR, "%s: Error reading PROCFS entry \n", __func__);
    close(fd);
    return 0;
  }

  close(fd);
  return 0;
}

int get_memory(uint8_t *output) {
  struct sysinfo info;
  sysinfo(&info);

  snprintf((char *)output, MAX_MESSAGE_SIZE,
           "Total:%luM\nFree:%luM\nShared:%luK\nBuffer:%luK\nProcs:%i\n",
           (info.totalram / 1024 / 1024), (info.freeram / 1024 / 1024),
           (info.sharedram / 1024), (info.bufferram / 1024), info.procs);

  return 0;
}

void set_custom_modem_name(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  char name[32];
  offset = (uint8_t *)strstr((char *)command, partial_commands[0].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error setting my new name\n");
  } else {
    int ofs = (int)(offset - command) + strlen(partial_commands[0].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(name, 32, "%s", (char *)command + ofs);
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "My name is now %s\n",
                       name);
      set_modem_name(name);
      get_names();
    }
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void set_custom_user_name(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  char name[32];
  offset = (uint8_t *)strstr((char *)command, partial_commands[1].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error setting your new name\n");
  } else {
    int ofs = (int)(offset - command) + strlen(partial_commands[1].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(name, 32, "%s", (char *)command + ofs);
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "I will call you %s from now on\n", name);
      set_user_name(name);
      get_names();
    }
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void delete_task(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  int taskID;
  char command_args[64];
  offset = (uint8_t *)strstr((char *)command, partial_commands[5].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Command mismatch!\n");
  } else {
    int ofs = (int)(offset - command) + strlen(partial_commands[5].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(command_args, 64, "%s", (char *)command + ofs);
      taskID = atoi((char *)command + ofs);
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "Removed task %i \n",
                       taskID);
      if (remove_task(taskID) < 0) {
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Remove task %i failed: It doesn't exist!\n", taskID);
      }
    }
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}
void dump_signal_report() {
  int strsz = 0;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  struct cell_report report = get_current_cell_report();
  if (report.net_type < 0) {
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Serving cell report has not been retrieved yet or is invalid\n");
  } else {
    switch (report.net_type) {
    case 0: // GSM
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "GSM Report: %i-%i\n",
                       report.mcc, report.mnc);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Cell: %s\n", report.cell_id);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "lac %s\n", report.gsm.lac);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "arfcn %i\n", report.gsm.arfcn);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "band %i\n", report.gsm.band);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rxlev %i\n", report.gsm.rxlev);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "txp %i\n", report.gsm.txp);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rla %i\n", report.gsm.rla);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "drx %i\n", report.gsm.drx);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "c1 %i\n", report.gsm.c1);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "c2 %i\n", report.gsm.c2);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "gprs %i\n", report.gsm.gprs);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "tch %i\n", report.gsm.tch);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "ts %i\n", report.gsm.ts);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "ta %i\n", report.gsm.ta);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "maio %i\n", report.gsm.maio);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "hsn %i\n", report.gsm.hsn);
      add_message_to_queue(reply, strsz);
      memset(reply, 0, MAX_MESSAGE_SIZE);
      strsz = snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                       "rxlevsub %i\n", report.gsm.rxlevsub);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rxlevfull %i\n", report.gsm.rxlevfull);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rxqualsub %i\n", report.gsm.rxqualsub);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rxqualfull %i\n", report.gsm.rxqualfull);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "voicecodec %i\n", report.gsm.voicecodec);

      break;

    case 1: // WCDMA
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "WCDMA Report: %i-%i\n",
                       report.mcc, report.mnc);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Cell: %s\n", report.cell_id);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "lac %s\n", report.wcdma.lac);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "uarfcn %i\n", report.wcdma.uarfcn);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "psc %i\n", report.wcdma.psc);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rac %i\n", report.wcdma.rac);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rscp %i\n", report.wcdma.rscp);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "ecio %i\n", report.wcdma.ecio);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "phych %i\n", report.wcdma.phych);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "sf %i\n", report.wcdma.sf);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "slot %i\n", report.wcdma.slot);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "speech codec %i\n", report.wcdma.speech_codec);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "conmod %i\n", report.wcdma.conmod);

      break;

    case 2: // LTE
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "LTE Report: %i-%i\n",
                       report.mcc, report.mnc);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Cell: %s\n", report.cell_id);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "is_tdd %i\n", report.lte.is_tdd);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "pcid %i\n", report.lte.pcid);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "earfcn %i\n", report.lte.earfcn);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "freq band ind %i\n", report.lte.freq_band_ind);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "ul bw %i\n", report.lte.ul_bandwidth);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "dl bw %i\n", report.lte.dl_bandwidth);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "tac %i\n", report.lte.tac);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rsrp %i\n", report.lte.rsrp);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rsrq %i\n", report.lte.rsrq);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "rssi %i\n", report.lte.rssi);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "sinr %i\n", report.lte.sinr);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "srxlev %i\n", report.lte.srxlev);
      break;

    default:
      strsz = snprintf(
          (char *)reply, MAX_MESSAGE_SIZE,
          "Serving cell report has not been retrieved yet or is invalid\n");
      break;
    }
  }
  add_message_to_queue(reply, strsz);

  free(reply);
  reply = NULL;
}

void *delayed_shutdown() {
  sleep(5);
  reboot(0x4321fedc);
  return NULL;
}

void *delayed_reboot() {
  sleep(5);
  reboot(0x01234567);
  return NULL;
}

void *schedule_call(void *cmd) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *command = (uint8_t *)cmd;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  logger(MSG_WARN, "SCH: %s -> %s \n", cmd, command);

  int delaysec;
  char tmpbuf[10];
  char *secoffset;
  offset = (uint8_t *)strstr((char *)command, partial_commands[2].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error reading the command\n");
    add_message_to_queue(reply, strsz);
  } else {
    int ofs = (int)(offset - command) + strlen(partial_commands[2].cmd);
    snprintf(tmpbuf, 10, "%s", (char *)command + ofs);
    delaysec = strtol(tmpbuf, &secoffset, 10);
    if (delaysec > 0) {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "I will call you back in %i seconds\n", delaysec);
      add_message_to_queue(reply, strsz);
      sleep(delaysec);
      logger(MSG_INFO, "Calling you now!\n");
      set_pending_call_flag(true);
    } else {
      strsz = snprintf(
          (char *)reply, MAX_MESSAGE_SIZE,
          "Please tell me in how many seconds you want me to call you, %s\n",
          cmd_runtime.user_name);
      add_message_to_queue(reply, strsz);
    }
  }
  free(reply);
  reply = NULL;
  command = NULL;
  return NULL;
}

void render_gsm_signal_data() {
  int strsz = 0;
  struct network_state netstat;
  netstat = get_network_status();
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Network type: ");
  if (get_network_type() >= 0x00 && get_network_type() <= 0x08) {
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s\n",
                      network_types[get_network_type()]);
  } else {
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Unknown (0x%.2x)\n", get_network_type());
  }
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Signal strength: %i %%\n", get_signal_strength());
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "%s roaming \n", netstat.is_roaming ? "Is " : "Not ");
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "%s in call \n", netstat.in_call ? "Is" : "Isn't");
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

/* Syntax:
 * Remind me in X[hours]:[optional minutes] [optional description spoken by tts]
 * Remind me at X[hours]:[optional minutes] [optional description spoken by tts]
 * Examples:
 *  remind me at 5 do some stuff
 *  remind me at 5:05 do some stuff
 *  remind me at 15:05 do some stuff
 *  remind me in 1 do some stuff
 *  remind me in 99 do some stuff
 *
 */
void schedule_reminder(uint8_t *command) {
  uint8_t *offset_command;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  int strsz = 0;
  char temp_str[160];
  char reminder_text[160] = {0};
  char current_word[160] = {0};
  int markers[128] = {0};
  int phrase_size = 1;
  int start = 0;
  int end = 0;
  char sep[] = " ";
  struct task_p scheduler_task;
  scheduler_task.time.mode = SCHED_MODE_TIME_AT; // 0 at, 1 in
  /* Initial command check */
  offset_command = (uint8_t *)strstr((char *)command, partial_commands[3].cmd);
  if (offset_command == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Command mismatch!\n");
    add_message_to_queue(reply, strsz);
    free(reply);
    reply = NULL;
    return;
  }

  strcpy(temp_str, (char *)command);
  int init_size = strlen(temp_str);
  char *ptr = strtok(temp_str, sep);
  while (ptr != NULL) {
    logger(MSG_INFO, "%s: '%s'\n", __func__, ptr);
    ptr = strtok(NULL, sep);
  }

  for (int i = 0; i < init_size; i++) {
    if (temp_str[i] == 0) {
      markers[phrase_size] = i;
      phrase_size++;
    }
  }

  logger(MSG_INFO, "%s: Total words in command: %i\n", __func__, phrase_size);
  for (int i = 0; i < phrase_size; i++) {
    start = markers[i];
    if (i + 1 >= phrase_size) {
      end = init_size;
    } else {
      end = markers[i + 1];
    }
    // So we don't pick the null byte separating the word
    if (i > 0) {
      start++;
    }
    // Copy this token
    memset(current_word, 0, 160);
    memcpy(current_word, temp_str + start, (end - start));
    // current_word[strlen(current_word)] = '\0';

    /*
     * remind me [at|in] 00[:00] [Text chunk]
     *  ^      ^   ^^^^  ^^  ^^     ^^^^^
     *  0      1     2   ?3 3OPT     4
     */

    logger(MSG_INFO, "Current word: %s\n", current_word);
    switch (i) {
    case 0:
      if (strstr(current_word, "remind") == NULL) {
        logger(MSG_ERROR, "%s: First word is not remind, %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "First word in command is wrong\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      } else {
        logger(MSG_INFO, "%s: Word ok: %s\n", __func__, current_word);
      }
      break;
    case 1:
      if (strstr(current_word, "me") == NULL) {
        logger(MSG_ERROR, "%s: Second word is not me, %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Second word in command is wrong\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      }
      break;
    case 2:
      if (strstr(current_word, "at") != NULL) {
        logger(MSG_INFO, "%s: Remind you AT \n", __func__);
      } else if (strstr((char *)command, "in") != NULL) {
        logger(MSG_INFO, "%s: Remind you IN \n", __func__);
        scheduler_task.time.mode = SCHED_MODE_TIME_COUNTDOWN;
      } else {
        logger(MSG_ERROR,
               "%s: Don't know when you want me to remind you: %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Do you want me to remind you *at* a specific time or "
                         "*in* some time from now\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      }
      break;
    case 3:
      if (strchr(current_word, ':') != NULL) {
        logger(MSG_INFO, "%s: Time has minutes", __func__);
        char *offset = strchr((char *)current_word, ':');
        scheduler_task.time.hh = get_int_from_str(current_word, 0);
        scheduler_task.time.mm = get_int_from_str(offset, 1);

        if (scheduler_task.time.mode == SCHED_MODE_TIME_AT) {
          if (scheduler_task.time.hh > 23 && scheduler_task.time.mm > 59) {
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Ha ha, very funny. Please give me a time value I "
                             "can work with if you want me to do anything \n");
          } else if (scheduler_task.time.hh > 23 ||
                     scheduler_task.time.mm > 59) {
            if (scheduler_task.time.hh > 23)
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "I might not be very smart, but I don't think "
                               "there's enough hours in a day \n");
            if (scheduler_task.time.mm > 59)
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "I might not be very smart, but I don't think "
                               "there's enough minutes in an hour \n");
            add_message_to_queue(reply, strsz);
            free(reply);
            reply = NULL;
            return;
          }
        }
      } else {
        logger(MSG_INFO, "%s:  Only hours have been specified\n", __func__);
        if (strlen(current_word) > 2) {
          logger(MSG_WARN, "%s: How long do you want me to wait? %s\n",
                 __func__, current_word);
          strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                           "I can't wait for a task that long... %s\n",
                           current_word);
          add_message_to_queue(reply, strsz);
          free(reply);
          reply = NULL;
          return;
        } else {
          scheduler_task.time.hh = atoi(current_word);
          scheduler_task.time.mm = 0;
          if (scheduler_task.time.mode)
            logger(MSG_WARN, "%s: Waiting for %i hours\n", __func__,
                   scheduler_task.time.hh);
          else
            logger(MSG_WARN, "%s: Waiting until %i to call you back\n",
                   __func__, scheduler_task.time.hh);
        }
      }
      break;
    case 4:
      memcpy(reminder_text, command + start, (strlen((char *)command) - start));
      logger(MSG_INFO, "%s: Reminder has the following text: %s\n", __func__,
             reminder_text);
      if (scheduler_task.time.mode == SCHED_MODE_TIME_AT) {
        strsz = snprintf(
            (char *)reply, MAX_MESSAGE_SIZE, " Remind you at %.2i:%.2i.\n%s\n",
            scheduler_task.time.hh, scheduler_task.time.mm, reminder_text);
      } else if (scheduler_task.time.mode == SCHED_MODE_TIME_COUNTDOWN) {
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         " Remind you in %i hours and %i minutes\n%s\n",
                         scheduler_task.time.hh, scheduler_task.time.mm,
                         reminder_text);
      }
      break;
    }
  }
  scheduler_task.type = TASK_TYPE_CALL;
  scheduler_task.status = STATUS_PENDING;
  scheduler_task.param = 0;
  scheduler_task.time.mode = scheduler_task.time.mode;
  strncpy(scheduler_task.arguments, reminder_text, MAX_MESSAGE_SIZE);
  if (add_task(scheduler_task) < 0) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     " Can't add reminder, my task queue is full!\n");
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

/* Syntax:
 * Wake me up in X[hours]:[optional minutes]
 * Wake me up at X[hours]:[optional minutes]
 * Examples:
 *  wake me up at 5
 *  wake me up at 5:05
 *  wake me up at 15:05
 *  wake me up in 1
 *  wake me up in 99
 *
 */
void schedule_wakeup(uint8_t *command) {
  uint8_t *offset_command;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  int strsz = 0;
  char temp_str[160];
  char current_word[160] = {0};
  int markers[128] = {0};
  int phrase_size = 1;
  int start = 0;
  int end = 0;
  char sep[] = " ";
  struct task_p scheduler_task;
  scheduler_task.time.mode = SCHED_MODE_TIME_AT; // 0 at, 1 in
  /* Initial command check */
  offset_command = (uint8_t *)strstr((char *)command, partial_commands[4].cmd);
  if (offset_command == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Command mismatch!\n");
    add_message_to_queue(reply, strsz);
    free(reply);
    reply = NULL;
    return;
  }

  strcpy(temp_str, (char *)command);
  int init_size = strlen(temp_str);
  char *ptr = strtok(temp_str, sep);
  while (ptr != NULL) {
    logger(MSG_INFO, "%s: '%s'\n", __func__, ptr);
    ptr = strtok(NULL, sep);
  }

  for (int i = 0; i < init_size; i++) {
    if (temp_str[i] == 0) {
      markers[phrase_size] = i;
      phrase_size++;
    }
  }

  logger(MSG_INFO, "%s: Total words in command: %i\n", __func__, phrase_size);
  for (int i = 0; i < phrase_size; i++) {
    start = markers[i];
    if (i + 1 >= phrase_size) {
      end = init_size;
    } else {
      end = markers[i + 1];
    }
    // So we don't pick the null byte separating the word
    if (i > 0) {
      start++;
    }
    // Copy this token
    memset(current_word, 0, 160);
    memcpy(current_word, temp_str + start, (end - start));
    // current_word[strlen(current_word)] = '\0';

    /*
     * wake me up [at|in] 00[:00]
     *  ^    ^  ^   ^^^^  ^^  ^^
     *  0    1  2     3   ?4 3OPT
     */

    logger(MSG_INFO, "Current word: %s\n", current_word);
    switch (i) {
    case 0:
      if (strstr(current_word, "wake") == NULL) {
        logger(MSG_ERROR, "%s: First word is not wake, %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "First word in command is wrong\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      } else {
        logger(MSG_INFO, "%s: Word ok: %s\n", __func__, current_word);
      }
      break;
    case 1:
      if (strstr(current_word, "me") == NULL) {
        logger(MSG_ERROR, "%s: Second word is not me, %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Second word in command is wrong\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      }
      break;
    case 2:
      if (strstr(current_word, "up") == NULL) {
        logger(MSG_ERROR, "%s: Third word is not up, %s \n", __func__,
               current_word);
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Second word in command is wrong\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      }
      break;
    case 3:
      if (strstr(current_word, "at") != NULL) {
        logger(MSG_INFO, "%s: Wake you AT \n", __func__);
      } else if (strstr((char *)command, "in") != NULL) {
        logger(MSG_INFO, "%s: Wake you IN \n", __func__);
        scheduler_task.time.mode = SCHED_MODE_TIME_COUNTDOWN;
      } else {
        logger(MSG_ERROR,
               "%s: Don't know when you want me to wake you up: %s \n",
               __func__, current_word);
        strsz =
            snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Do you want me to wake you up *at* a specific time or "
                     "*in* some time from now?\n");
        add_message_to_queue(reply, strsz);
        free(reply);
        reply = NULL;
        return;
      }
      break;
    case 4:
      if (strchr(current_word, ':') != NULL) {
        logger(MSG_INFO, "%s: Time has minutes", __func__);
        char *offset = strchr((char *)current_word, ':');
        scheduler_task.time.hh = get_int_from_str(current_word, 0);
        scheduler_task.time.mm = get_int_from_str(offset, 1);

        if (offset - ((char *)current_word) > 2 || strlen(offset) > 3) {
          logger(MSG_WARN, "%s: How long do you want me to wait? %s\n",
                 __func__, current_word);
          if (offset - ((char *)current_word) > 2)
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "I can't wait for a task that long... %s\n",
                             current_word);
          if (strlen(offset) > 3)
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Add another hour rather than putting 100 or more "
                             "minutes... %s\n",
                             current_word);
          add_message_to_queue(reply, strsz);
          free(reply);
          reply = NULL;
          return;
        } else if (scheduler_task.time.mode == SCHED_MODE_TIME_AT) {
          if (scheduler_task.time.hh > 23 && scheduler_task.time.mm > 59) {
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Ha ha, very funny. Please give me a time value I "
                             "can work with if you want me to do anything \n");
          } else if (scheduler_task.time.hh > 23 ||
                     scheduler_task.time.mm > 59) {
            if (scheduler_task.time.hh > 23)
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "I might not be very smart, but I don't think "
                               "there's enough hours in a day \n");
            if (scheduler_task.time.mm > 59)
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "I might not be very smart, but I don't think "
                               "there's enough minutes in an hour \n");
            add_message_to_queue(reply, strsz);
            free(reply);
            reply = NULL;
            return;
          } else {
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Waiting until %i:%02i to call you back\n",
                             scheduler_task.time.hh, scheduler_task.time.mm);
          }
        } else {
          strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                           "Waiting for %i hours and %i minutes to call you "
                           "back\n",
                           scheduler_task.time.hh, scheduler_task.time.mm);
        }
      } else {
        logger(MSG_INFO, "%s:  Only hours have been specified\n", __func__);
        if (strlen(current_word) > 2) {
          logger(MSG_WARN, "%s: How long do you want me to wait? %s\n",
                 __func__, current_word);
          strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                           "I can't wait for a task that long... %s\n",
                           current_word);
          add_message_to_queue(reply, strsz);
          free(reply);
          reply = NULL;
          return;
        } else {
          scheduler_task.time.hh = atoi(current_word);
          scheduler_task.time.mm = 0;
          if (scheduler_task.time.mode) {
            logger(MSG_WARN, "%s: Waiting for %i hours\n", __func__,
                   scheduler_task.time.hh);
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Waiting for %i hours to call you back\n",
                             scheduler_task.time.hh);
          } else {
            if (scheduler_task.time.hh > 24) {
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "I might not be very smart, but I don't think "
                               "there's enough hours in a day \n");
              add_message_to_queue(reply, strsz);
              free(reply);
              reply = NULL;
              return;
            } else {
              logger(MSG_WARN, "%s: Waiting until %i to call you back\n",
                     __func__, scheduler_task.time.hh);
              strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "Waiting until %i to call you back\n",
                               scheduler_task.time.hh);
            }
          }
        }
      }
      break;
    }
  }
  scheduler_task.type = TASK_TYPE_CALL;
  scheduler_task.status = STATUS_PENDING;
  scheduler_task.param = 0;
  scheduler_task.time.mode = scheduler_task.time.mode;
  snprintf(scheduler_task.arguments, MAX_MESSAGE_SIZE,
           "It's time to wakeup, %s", cmd_runtime.user_name);
  if (add_task(scheduler_task) < 0) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     " Can't schedule wakeup, my task queue is full!\n");
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void set_cb_broadcast(bool en) {
  char *response = malloc(128 * sizeof(char));
  uint8_t *reply = calloc(160, sizeof(unsigned char));
  int strsz;
  int cmd_ret;
  if (en) {
    cmd_ret = send_at_command(CB_ENABLE_AT_CMD, sizeof(CB_ENABLE_AT_CMD),
                              response, 128);
    if (cmd_ret < 0 || (strstr(response, "OK") == NULL)) {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Failed to enable cell broadcasting messages\n");
    } else {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Enabling Cell broadcast messages\n");
    }
  } else {
    cmd_ret = send_at_command(CB_DISABLE_AT_CMD, sizeof(CB_DISABLE_AT_CMD),
                              response, 128);
    if (cmd_ret < 0 || (strstr(response, "OK") == NULL)) {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Failed to disable cell broadcasting messages\n");
    } else {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Disabling Cell broadcast messages\n");
    }
  }
  add_message_to_queue(reply, strsz);

  free(response);
  free(reply);
  response = NULL;
  reply = NULL;
}

uint8_t parse_command(uint8_t *command) {
  int ret = 0;
  uint16_t i, random;
  FILE *fp;
  int cmd_id = -1;
  int strsz = 0;
  struct pkt_stats packet_stats;
  pthread_t disposable_thread;
  char lowercase_cmd[160];
  uint8_t *tmpbuf = calloc(MAX_MESSAGE_SIZE, sizeof(unsigned char));
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(unsigned char));
  srand(time(NULL));
  for (i = 0; i < command[i]; i++) {
    lowercase_cmd[i] = tolower(command[i]);
  }
  lowercase_cmd[strlen((char *)command)] = '\0';
  /* Static commands */
  for (i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0])); i++) {
    if ((strcmp((char *)command, bot_commands[i].cmd) == 0) ||
        (strcmp(lowercase_cmd, bot_commands[i].cmd) == 0)) {
      logger(MSG_DEBUG, "%s: Match! %s\n", __func__, bot_commands[i].cmd);
      cmd_id = bot_commands[i].id;
    }
  }

  /* Commands with arguments */
  if (cmd_id == -1) {
    for (i = 0; i < (sizeof(partial_commands) / sizeof(partial_commands[0]));
         i++) {
      if ((strstr((char *)command, partial_commands[i].cmd) != NULL) ||
          (strstr((char *)lowercase_cmd, partial_commands[i].cmd) != NULL)) {
        cmd_id = partial_commands[i].id;
      }
    }
  }
  ret = find_cmd_history_match(cmd_id);
  if (ret >= 5) {
    logger(MSG_WARN, "You're pissing me off\n");
    random = rand() % 10;
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s\n",
                      repeated_cmd[random].answer);
  }
  switch (cmd_id) {
  case -1:
    logger(MSG_INFO, "%s: Nothing to do\n", __func__);
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Command not found: %s\n", command);
    add_message_to_queue(reply, strsz);
    break;
  case 0:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.bot_name);
    add_message_to_queue(reply, strsz);
    break;
  case 1:
    if (get_uptime(tmpbuf) == 0) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Hi %s, %s:\n %s\n", cmd_runtime.user_name,
                        bot_commands[cmd_id].cmd_text, tmpbuf);
    } else {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Error getting the uptime\n");
    }
    add_message_to_queue(reply, strsz);
    break;
  case 2:
    if (get_load_avg(tmpbuf) == 0) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Hi %s, %s:\n %s\n", cmd_runtime.user_name,
                        bot_commands[cmd_id].cmd_text, tmpbuf);
    } else {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Error getting laodavg\n");
    }
    add_message_to_queue(reply, strsz);
    break;
  case 3:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "I'm at version %s\n"
                      "ADSP Version is %s\n",
                      RELEASE_VER, known_adsp_fw[read_adsp_version()].fwstring);
    add_message_to_queue(reply, strsz);
    break;
  case 4:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                 "USB Suspend state: %i\n", get_transceiver_suspend_state());
    add_message_to_queue(reply, strsz);
    break;
  case 5:
    if (get_memory(tmpbuf) == 0) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Memory stats:\n%s\n", tmpbuf);
    } else {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Error getting laodavg\n");
    }
    add_message_to_queue(reply, strsz);
    break;
  case 6:
    packet_stats = get_rmnet_stats();
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "RMNET IF stats:\nBypassed: "
                      "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%i",
                      packet_stats.bypassed, packet_stats.empty,
                      packet_stats.discarded, packet_stats.failed,
                      packet_stats.allowed);
    add_message_to_queue(reply, strsz);
    break;
  case 7:
    packet_stats = get_gps_stats();
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "GPS IF stats:\nBypassed: "
                      "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%"
                      "i\nQMI Location svc.: %i",
                      packet_stats.bypassed, packet_stats.empty,
                      packet_stats.discarded, packet_stats.failed,
                      packet_stats.allowed, packet_stats.other);
    add_message_to_queue(reply, strsz);
    break;
  case 8:
    strsz = 0;
    snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
             "Help: Static commands\n");
    add_message_to_queue(reply, strsz);
    strsz = 0;
    for (i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0])); i++) {
      if (strlen(bot_commands[i].cmd) + (3 * sizeof(uint8_t)) +
              strlen(bot_commands[i].help) + strsz >
          MAX_MESSAGE_SIZE) {
        add_message_to_queue(reply, strsz);
        strsz = 0;
      }
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "%s: %s\n", bot_commands[i].cmd, bot_commands[i].help);
    }
    add_message_to_queue(reply, strsz);
    strsz = 0;
    snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
             "Help: Commands with arguments\n");
    add_message_to_queue(reply, strsz);
    strsz = 0;
    for (i = 0; i < (sizeof(partial_commands) / sizeof(partial_commands[0]));
         i++) {
      if (strlen(partial_commands[i].cmd) + (5 * sizeof(uint8_t)) +
              strlen(partial_commands[i].help) + strsz >
          MAX_MESSAGE_SIZE) {
        add_message_to_queue(reply, strsz);
        strsz = 0;
      }
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "%s x: %s\n", partial_commands[i].cmd,
                        partial_commands[i].help);
    }
    add_message_to_queue(reply, strsz);
    break;
  case 9:
    strsz += snprintf(
        (char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
        "Blocking USB suspend until reboot or until you tell me otherwise!\n");
    set_suspend_inhibit(true);
    add_message_to_queue(reply, strsz);
    break;
  case 10:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Allowing USB to suspend again\n");
    set_suspend_inhibit(false);
    add_message_to_queue(reply, strsz);
    break;
  case 11:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Turning ADB *ON*\n");
    store_adb_setting(true);
    restart_usb_stack();
    add_message_to_queue(reply, strsz);
    break;
  case 12:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Turning ADB *OFF*\n");
    store_adb_setting(false);
    restart_usb_stack();
    add_message_to_queue(reply, strsz);
    break;
  case 13:
    for (i = 0; i < cmd_runtime.cmd_position; i++) {
      if (strsz < 160) {
        strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                          "%i ", cmd_runtime.cmd_history[i]);
      }
    }
    add_message_to_queue(reply, strsz);
    break;
  case 14:
    fp = fopen("/var/log/openqti.log", "r");
    if (fp == NULL) {
      logger(MSG_ERROR, "%s: Error opening file \n", __func__);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Error opening file\n");
    } else {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "OpenQTI Log\n");
      add_message_to_queue(reply, strsz);
      if (ret > (MAX_MESSAGE_SIZE * QUEUE_SIZE)) {
        fseek(fp, (ret - (MAX_MESSAGE_SIZE * QUEUE_SIZE)), SEEK_SET);
      } else {
        fseek(fp, 0L, SEEK_SET);
      }
      do {
        memset(reply, 0, MAX_MESSAGE_SIZE);
        ret = fread(reply, 1, MAX_MESSAGE_SIZE - 2, fp);
        if (ret > 0) {
          add_message_to_queue(reply, ret);
        }
      } while (ret > 0);
      fclose(fp);
    }
    break;
  case 15:
    fp = fopen("/var/log/messages", "r");
    if (fp == NULL) {
      logger(MSG_ERROR, "%s: Error opening file \n", __func__);
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Error opening file\n");
    } else {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "DMESG:\n");
      add_message_to_queue(reply, strsz);
      fseek(fp, 0L, SEEK_END);
      ret = ftell(fp);
      if (ret > (MAX_MESSAGE_SIZE * QUEUE_SIZE)) {
        fseek(fp, (ret - (MAX_MESSAGE_SIZE * QUEUE_SIZE)), SEEK_SET);
      } else {
        fseek(fp, 0L, SEEK_SET);
      }
      do {
        memset(reply, 0, MAX_MESSAGE_SIZE);
        ret = fread(reply, 1, MAX_MESSAGE_SIZE - 2, fp);
        if (ret > 0) {
          add_message_to_queue(reply, ret);
        }
      } while (ret > 0);
      fclose(fp);
    }
    break;

  case 16:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s: %i\n",
                 bot_commands[cmd_id].cmd_text, get_dirty_reconnects());
    add_message_to_queue(reply, strsz);
    break;
  case 17:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s\n",
                      bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_pending_call_flag(true);
    break;
  case 18:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.user_name);
    add_message_to_queue(reply, strsz);
    break;
  case 19:
    pthread_create(&disposable_thread, NULL, &delayed_shutdown, NULL);
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s!\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.user_name);
    add_message_to_queue(reply, strsz);
    break;
  case 20:
    render_gsm_signal_data();
    break;
  case 21:
    pthread_create(&disposable_thread, NULL, &delayed_reboot, NULL);
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s!\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.user_name);
    add_message_to_queue(reply, strsz);
    break;
  case 22:
    dump_signal_report();
    break;
  case 23:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    enable_signal_tracking(true);
    break;
  case 24:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    enable_signal_tracking(false);
    break;
  case 25:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_persistent_logging(true);
    break;
  case 26:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_persistent_logging(false);
    break;
  case 27:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_sms_logging(true);
    break;
  case 28:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_sms_logging(false);
    break;
  case 29: /* List pending tasks */
    dump_pending_tasks();
    break;
  case 30:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Hi, my name is %s and I'm at version %s\n",
                      cmd_runtime.bot_name, RELEASE_VER);
    add_message_to_queue(reply, strsz);
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "    .-.     .-.\n .****. .****.\n.*****.*****. Thank\n  .*********.   "
        " You\n    .*******.\n     .*****.\n       .***.\n          *\n");
    add_message_to_queue(reply, strsz);
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Thank you for using me!\n And, especially, for all of you...");
    add_message_to_queue(reply, strsz);
    fp = fopen("/usr/share/thank_you/thankyou.txt", "r");
    if (fp != NULL) {
      size_t len = 0;
      char *line;
      memset(reply, 0, MAX_MESSAGE_SIZE);
      strsz = 0;
      i = 0;
      while ((ret = getline(&line, &len, fp)) != -1) {
        strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s",
                          line);
        if (i > 15 || strsz > 140) {
          add_message_to_queue(reply, strsz);
          memset(reply, 0, MAX_MESSAGE_SIZE);
          strsz = 0;
          i = 0;
        }
        i++;
      }
      if (strsz > 0) {
        add_message_to_queue(reply, strsz);
      }
      fclose(fp);
      if (line) {
        free(line);
      }
    }
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Thank you for supporting my development and improving me "
                     "every day, I wouldn't have a purpose without you all!");
    add_message_to_queue(reply, strsz);
    break;
  case 31:
    set_cb_broadcast(true);
    break;
  case 32:
    set_cb_broadcast(false);
    break;
  case 33:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    enable_call_waiting_autohangup(2);
    break;
  case 34:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    enable_call_waiting_autohangup(1);
    break;
  case 35:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    enable_call_waiting_autohangup(0);
    break;
  case 36:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_custom_alert_tone(true); // enable in runtime
    break;
  case 37:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                     bot_commands[cmd_id].cmd_text);
    add_message_to_queue(reply, strsz);
    set_custom_alert_tone(false); // enable in runtime
    break;


        //    

  case 100:
    set_custom_modem_name(command);
    break;
  case 101:
    set_custom_user_name(command);
    break;
  case 102:
    pthread_create(&disposable_thread, NULL, &schedule_call, command);
    sleep(2); // our string gets wiped out before we have a chance
    break;
  case 103:
    schedule_reminder(command);
    break;
  case 104:
    schedule_wakeup(command);
    break;
  case 105: /* Delete task %i */
    delete_task(command);

    break;
  default:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Invalid command id %i\n", cmd_id);
    logger(MSG_INFO, "%s: Unknown command %i\n", __func__, cmd_id);
    break;
  }

  add_to_history(cmd_id);

  free(tmpbuf);
  free(reply);

  tmpbuf = NULL;
  reply = NULL;
  return ret;
}
