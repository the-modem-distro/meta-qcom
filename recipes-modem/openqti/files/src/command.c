// SPDX-License-Identifier: MIT

#include "../inc/command.h"
#include "../inc/logger.h"
#include "../inc/proxy.h"
#include "../inc/sms.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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

void set_cmd_runtime_defaults() {
  cmd_runtime.is_unlocked = false;
  cmd_runtime.unlock_time = 0;
  strncpy(cmd_runtime.user_name, "User",
          32); // FIXME: Allow user to set a custom name
  strncpy(cmd_runtime.bot_name, "Modem",
          32); // FIXME: Allow to change modem name
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

uint8_t parse_command(uint8_t *command) {
  int ret = 0;
  uint16_t i, random;
  FILE *fp;
  int cmd_id = -1;
  int strcount = 0;
  struct pkt_stats packet_stats;
  uint8_t *tmpbuf = calloc(128, sizeof(unsigned char));
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  srand(time(NULL));
  set_cmd_runtime_defaults();
  for (i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0])); i++) {
    if (strcmp((char *)command, bot_commands[i].cmd) == 0) {
      cmd_id = bot_commands[i].id;
    }
  }
  ret = find_cmd_history_match(cmd_id);
  logger(MSG_INFO, "Repeated cmds %i\n", ret);
  if (ret >= 5) {
    logger(MSG_WARN, "You're pissing me off\n");
    random = rand() % 10;
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "%s\n", repeated_cmd[random].answer);
  }
  switch (cmd_id) {
  case -1:
    logger(MSG_INFO, "%s: Nothing to do\n", __func__);
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Command not found: %s\n", command);
    add_message_to_queue(reply, strcount);
    break;
  case 0:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "%s %s\n", bot_commands[cmd_id].cmd_text,
                         cmd_runtime.bot_name);
    add_message_to_queue(reply, strcount);
    break;
  case 1:
    if (get_uptime(tmpbuf) == 0) {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "This is my uptime:\n %s\n", tmpbuf);
    } else {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "Error getting the uptime\n");
    }
    add_message_to_queue(reply, strcount);
    break;
  case 2:
    if (get_load_avg(tmpbuf) == 0) {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "My current load avg is: %s\n", tmpbuf);
    } else {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "Error getting laodavg\n");
    }
    add_message_to_queue(reply, strcount);
    break;
  case 3:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "I'm at version %s\n", RELEASE_VER);
    add_message_to_queue(reply, strcount);
    break;
  case 4:
    strcount +=
        snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                 "USB Suspend state: %i\n", get_transceiver_suspend_state());
    add_message_to_queue(reply, strcount);
    break;
  case 5:
    if (get_memory(tmpbuf) == 0) {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "Memory stats:\n%s\n", tmpbuf);
    } else {
      strcount +=
          snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                   "Error getting laodavg\n");
    }
    add_message_to_queue(reply, strcount);
    break;
  case 6:
    packet_stats = get_rmnet_stats();
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "RMNET IF stats:\nBypassed: "
                         "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%i",
                         packet_stats.bypassed, packet_stats.empty,
                         packet_stats.discarded, packet_stats.failed,
                         packet_stats.allowed);
    add_message_to_queue(reply, strcount);
    break;
  case 7:
    packet_stats = get_gps_stats();
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "GPS IF stats:\nBypassed: "
                         "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%"
                         "i\nQMI Location svc.: %i",
                         packet_stats.bypassed, packet_stats.empty,
                         packet_stats.discarded, packet_stats.failed,
                         packet_stats.allowed, packet_stats.other);
    add_message_to_queue(reply, strcount);
    break;
  case 8:
    strcount = 0;
    for (i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0])); i++) {
    if (strlen(bot_commands[i].cmd) + 
        (3* sizeof(uint8_t)) + strlen(bot_commands[i].help) + 
        strcount > MAX_MESSAGE_SIZE) {
          add_message_to_queue(reply, strcount);
          strcount = 0;
        }
        strcount+= snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount, "%s: %s\n", bot_commands[i].cmd, bot_commands[i].help );
    }
    add_message_to_queue(reply, strcount);
    break;
  case 9:
    strcount += snprintf(
        (char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
        "Blocking USB suspend until reboot or until you tell me otherwise!\n");
    set_suspend_inhibit(false);
    add_message_to_queue(reply, strcount);
    break;
  case 10:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Allowing USB tu suspend again\n");
    set_suspend_inhibit(false);
    add_message_to_queue(reply, strcount);
    break;
  case 11:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Turning ADB *ON*\n");
    store_adb_setting(true);
    restart_usb_stack();
    add_message_to_queue(reply, strcount);
    break;
  case 12:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Turning ADB *OFF*\n");
    store_adb_setting(false);
    restart_usb_stack();
    add_message_to_queue(reply, strcount);
    break;
  case 13:
    for (i = 0; i < cmd_runtime.cmd_position; i++) {
      if (strcount < 160) {
        strcount +=
            snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                     "%i ", cmd_runtime.cmd_history[i]);
      }
    }
    add_message_to_queue(reply, strcount);
    break;
    case 14:
    fp = fopen("/var/log/openqti.log", "r");
    if (fp == NULL) {
      logger(MSG_ERROR, "%s: Error opening file \n", __func__);
          strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Error opening file\n");
    } else {
      strcount = snprintf((char *)reply , MAX_MESSAGE_SIZE, "OpenQTI Log\n");
      add_message_to_queue(reply, strcount);
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
          strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
                         "Error opening file\n");
    } else {
      strcount = snprintf((char *)reply , MAX_MESSAGE_SIZE, "DMESG:\n");
      add_message_to_queue(reply, strcount);
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
  default:
    strcount += snprintf((char *)reply + strcount, MAX_MESSAGE_SIZE - strcount,
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
