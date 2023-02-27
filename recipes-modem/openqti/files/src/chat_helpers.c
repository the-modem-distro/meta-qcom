/* SPDX-License-Identifier: MIT */

#include "chat_helpers.h"
#include "adspfw.h"
#include "audio.h"
#include "call.h"
#include "cell_broadcast.h"
#include "command.h"
#include "config.h"
#include "dms.h"
#include "ipc.h"
#include "logger.h"
#include "nas.h"
#include "proxy.h"
#include "scheduler.h"
#include "sms.h"
#include "tracking.h"
#include "wds.h"
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

void get_rmnet_stats_cmd() {
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  struct pkt_stats packet_stats;

  packet_stats = get_rmnet_stats();
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "RMNET IF stats:\nBypassed: "
                    "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%i",
                    packet_stats.bypassed, packet_stats.empty,
                    packet_stats.discarded, packet_stats.failed,
                    packet_stats.allowed);
  add_message_to_queue(reply, strsz);
}
void get_gps_stats_cmd() {
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  struct pkt_stats packet_stats;

  packet_stats = get_gps_stats();
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "GPS IF stats:\nBypassed: "
                    "%i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%"
                    "i\nQMI Location svc.: %i",
                    packet_stats.bypassed, packet_stats.empty,
                    packet_stats.discarded, packet_stats.failed,
                    packet_stats.allowed, packet_stats.other);
  add_message_to_queue(reply, strsz);
}

void cmd_get_help() {
  /* Help */
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];

  char *full_help_msg[MSG_MAX_MULTIPART_SIZE] = { 0 };
  size_t full_msg_size = 0;
  full_msg_size =
      snprintf((char *)full_help_msg, MSG_MAX_MULTIPART_SIZE,
               "Welcome to the modem distro!\n"
               "You can use the chat to view and change a lot of settings\n"
               "Here's a list of what you can do:\n");
  for (uint8_t j = 0; j < (sizeof(cmd_categories) / sizeof(cmd_categories[0]));
       j++) {
    full_msg_size += snprintf((char *)full_help_msg + full_msg_size,
                              MSG_MAX_MULTIPART_SIZE - full_msg_size,
                              "Category: %s\n", cmd_categories[j].name);
    for (uint8_t i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0]));
         i++) {
      if (cmd_categories[j].category == bot_commands[i].category) {
        if (bot_commands[i].is_partial) {
          full_msg_size +=
              snprintf((char *)full_help_msg + full_msg_size,
                       MSG_MAX_MULTIPART_SIZE - full_msg_size, "%s x: %s\n",
                       bot_commands[i].cmd, bot_commands[i].help);
        } else {
          full_msg_size +=
              snprintf((char *)full_help_msg + full_msg_size,
                       MSG_MAX_MULTIPART_SIZE - full_msg_size, "%s: %s\n",
                       bot_commands[i].cmd, bot_commands[i].help);
        }
      }
    }
  }
  //  set_queue_lock(true);
  while (strsz < full_msg_size) {
    memset(reply, 0, MAX_MESSAGE_SIZE);
    if ((full_msg_size - strsz) > MAX_MESSAGE_SIZE) {
      memcpy((char *)reply, (full_help_msg + strsz), (MAX_MESSAGE_SIZE - 10));
      add_message_to_queue(reply, (MAX_MESSAGE_SIZE - 10));
      strsz += (MAX_MESSAGE_SIZE - 10);
    } else if ((full_msg_size - strsz) > 0) {
      memcpy((char *)reply, (full_help_msg + strsz), (full_msg_size - strsz));
      add_message_to_queue(reply, (full_msg_size - strsz));
      strsz += (full_msg_size - strsz);
    }
  }
  //  set_queue_lock(false);
}

int get_uptime() {
  unsigned updays, uphours, upminutes;
  struct sysinfo info;
  struct tm *current_time;
  time_t current_secs;
  int bytes_written = 0;
  time(&current_secs);
  current_time = localtime(&current_secs);
  uint8_t output[160];

  sysinfo(&info);

  bytes_written +=
      snprintf((char *)output + bytes_written, MAX_MESSAGE_SIZE - bytes_written,
               "Hi %s, %s:\n", get_rt_user_name(),
               bot_commands[CMD_ID_GET_UPTIME].cmd_text);

  bytes_written += snprintf((char *)output + bytes_written, MAX_MESSAGE_SIZE,
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

  add_message_to_queue(output, bytes_written);

  return 0;
}

int cmd_get_load_avg() {
  int fd;
  uint8_t reply[MAX_MESSAGE_SIZE];
  uint8_t output[64];
  size_t strsz = 0;
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

  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Hi %s, %s:\n %s\n", get_rt_user_name(),
                    bot_commands[CMD_ID_GET_LOADAVG].cmd_text, output);
  add_message_to_queue(reply, strsz);

  return 0;
}

int cmd_get_memory() {
  struct sysinfo info;
  sysinfo(&info);
  uint8_t output[MAX_MESSAGE_SIZE];
  size_t strsz = 0;
  strsz = snprintf(
      (char *)output, MAX_MESSAGE_SIZE,
      "Memory "
      "stats:\nTotal:%luM\nFree:%luM\nShared:%luK\nBuffer:%luK\nProcs:%i\n",
      (info.totalram / 1024 / 1024), (info.freeram / 1024 / 1024),
      (info.sharedram / 1024), (info.bufferram / 1024), info.procs);
  add_message_to_queue(output, strsz);
  return 0;
}

void set_custom_modem_name(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  char name[32];
  offset = (uint8_t *)strstr((char *)command,
                             bot_commands[CMD_ID_SET_MODEM_NAME].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error setting my new name\n");
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_SET_MODEM_NAME].cmd);
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

void enable_service_debugging_for_service_id(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  uint8_t service_id = 0;
  offset = (uint8_t *)strstr(
      (char *)command, bot_commands[CMD_ID_DEBUG_ENABLE_SERVICE_LOGGING].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error trying to find a matching ID\n");
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_DEBUG_ENABLE_SERVICE_LOGGING].cmd);
    if (strlen((char *)command) > ofs) {
      service_id = atoi((char *)(command + ofs));
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Enabling debugging of service id %u (%s) \n", service_id, get_qmi_service_name(service_id));
      enable_service_debugging(service_id);
    }
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void set_new_signal_tracking_mode(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  uint8_t mode = 0;
  const char *modes[] = {"0:Standalone learn", "1:Standalone enforce", "2:OpenCellID learn", "3:OpenCellID enforce"};
  offset = (uint8_t *)strstr((char *)command,
                             bot_commands[CMD_ID_ACTION_SET_ST_MODE].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error processing the command\n");
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_SET_ST_MODE].cmd);
    if (strlen((char *)command) > ofs) {
      mode = atoi((char *)(command + ofs));
      if (mode < 4) {
        strsz =
            snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Signal Tracking mode changed: %s\n", modes[mode]);
        set_signal_tracking_mode(mode);
      } else {
        strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                         "Available modes for signal tracking:\n"
                         "%s\n"
                         "%s\n"
                         "%s\n"
                         "%s\n"
                         "Your choice: %u\n",
                         modes[0], modes[1], modes[2], modes[3],  mode);
      }
    }
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void set_custom_user_name(uint8_t *command) {
  int strsz = 0;
  uint8_t *offset;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  char name[32];
  offset = (uint8_t *)strstr((char *)command,
                             bot_commands[CMD_ID_SET_OWNER_NAME].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error setting your new name\n");
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_SET_OWNER_NAME].cmd);
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
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  int taskID;
  char command_args[64];
  offset = (uint8_t *)strstr((char *)command,
                             bot_commands[CMD_ID_ACTION_DELETE_TASK].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Command mismatch!\n");
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_DELETE_TASK].cmd);
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

void debug_gsm7_cb_message(uint8_t *command) {
  int strsz = 0;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  uint8_t example_pkt1[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x02, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x16, 0xc4, 0x64, 0x71, 0x0a, 0x4a, 0x4e,
      0xa9, 0xa0, 0x62, 0xd2, 0x59, 0x04, 0x15, 0xa5, 0x50, 0xe9, 0x53, 0x58,
      0x75, 0x1e, 0x15, 0xc4, 0xb4, 0x0b, 0x24, 0x93, 0xb9, 0x62, 0x31, 0x97,
      0x0c, 0x26, 0x93, 0x81, 0x5a, 0xa0, 0x98, 0x4d, 0x47, 0x9b, 0x81, 0xaa,
      0x68, 0x39, 0xa8, 0x05, 0x22, 0x86, 0xe7, 0x20, 0xa1, 0x70, 0x09, 0x2a,
      0xcb, 0xe1, 0xf2, 0xb7, 0x98, 0x0e, 0x22, 0x87, 0xe7, 0x20, 0x77, 0xb9,
      0x5e, 0x06, 0x21, 0xc3, 0x6e, 0x72, 0xbe, 0x75, 0x0d, 0xcb, 0xdd, 0xad,
      0x69, 0x7e, 0x4e, 0x07, 0x16, 0x01, 0x00, 0x00};
  uint8_t example_pkt2[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x03, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x26, 0xe5, 0x36, 0x28, 0xed, 0x06, 0x25,
      0xd1, 0xf2, 0xb2, 0x1c, 0x24, 0x2d, 0x9f, 0xd3, 0x6f, 0xb7, 0x0b, 0x54,
      0x9c, 0x83, 0xc4, 0xe5, 0x39, 0xbd, 0x8c, 0xa6, 0x83, 0xd6, 0xe5, 0xb4,
      0xbb, 0x0c, 0x3a, 0x96, 0xcd, 0x61, 0xb4, 0xdc, 0x05, 0x12, 0xa6, 0xe9,
      0xf4, 0x32, 0x28, 0x7d, 0x76, 0xbf, 0xe5, 0xe9, 0xb2, 0xbc, 0xec, 0x06,
      0x4d, 0xd3, 0x65, 0x10, 0x39, 0x5d, 0x06, 0x39, 0xc3, 0x63, 0xb4, 0x3c,
      0x3d, 0x46, 0xd3, 0x5d, 0xa0, 0x66, 0x19, 0x2d, 0x07, 0x25, 0xdd, 0xe6,
      0xf7, 0x1c, 0x64, 0x06, 0x16, 0x01, 0x00, 0x00};
  uint8_t example_pkt3[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x04, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x36, 0x69, 0x37, 0xb9, 0xec, 0x06, 0x4d,
      0xd3, 0x65, 0x50, 0xdd, 0x4d, 0x2f, 0xcb, 0x41, 0x68, 0x3a, 0x1d, 0xae,
      0x7b, 0xbd, 0xee, 0xf7, 0xbb, 0x4b, 0x2c, 0x5e, 0xbb, 0xc4, 0x75, 0x37,
      0xd9, 0x45, 0x2e, 0xbf, 0xc6, 0x65, 0x36, 0x5b, 0x2c, 0x7f, 0x87, 0xc9,
      0xe3, 0xf0, 0x9c, 0x0e, 0x12, 0x82, 0xa6, 0x6f, 0x36, 0x9b, 0x5e, 0x76,
      0x83, 0xa6, 0xe9, 0x32, 0x88, 0x9c, 0x2e, 0xcf, 0xcb, 0x20, 0x67, 0x78,
      0x8c, 0x96, 0xa7, 0xc7, 0x68, 0x3a, 0xa8, 0x2c, 0x47, 0x87, 0xd9, 0xf4,
      0xb2, 0x9b, 0x05, 0x02, 0x16, 0x01, 0x00, 0x00};
  uint8_t example_pkt4[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x46, 0xf3, 0x37, 0xc8, 0x5e, 0x96, 0x9b,
      0xfd, 0x67, 0x3a, 0x28, 0x89, 0x96, 0x83, 0xa6, 0xed, 0xb0, 0x9c, 0x0e,
      0x47, 0xbf, 0xdd, 0x65, 0xd0, 0xf9, 0x6c, 0x76, 0x81, 0xdc, 0xe9, 0x31,
      0x9a, 0x0e, 0xf2, 0x8b, 0xcb, 0x72, 0x10, 0xb9, 0xec, 0x06, 0x85, 0xd7,
      0xf4, 0x7a, 0x99, 0xcd, 0x2e, 0xbb, 0x41, 0xd3, 0xb7, 0x99, 0x7e, 0x0f,
      0xcb, 0xcb, 0x73, 0x7a, 0xd8, 0x4d, 0x76, 0x81, 0xae, 0x69, 0x39, 0xa8,
      0xdc, 0x86, 0x9b, 0xcb, 0x68, 0x76, 0xd9, 0x0d, 0x22, 0x87, 0xd1, 0x65,
      0x39, 0x0b, 0x94, 0x04, 0x16, 0x01, 0x00, 0x00};
  uint8_t example_pkt5[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x06, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x56, 0x68, 0x39, 0xa8, 0xe8, 0x26, 0x9f,
      0xcb, 0xf2, 0x3d, 0x1d, 0x14, 0xae, 0x9b, 0x41, 0xe4, 0xb2, 0x1b, 0x14,
      0x5e, 0xd3, 0xeb, 0x65, 0x36, 0xbb, 0xec, 0x06, 0x4d, 0xdf, 0x66, 0xfa,
      0x3d, 0x2c, 0x2f, 0xcf, 0xe9, 0x61, 0x37, 0x19, 0xa4, 0xaf, 0x83, 0xe0,
      0x72, 0xbf, 0xb9, 0xec, 0x76, 0x81, 0x84, 0xe5, 0x34, 0xc8, 0x28, 0x0f,
      0x9f, 0xcb, 0x6e, 0x10, 0x3a, 0x5d, 0x96, 0xeb, 0xeb, 0xa0, 0x7b, 0xd9,
      0x4d, 0x2e, 0xbb, 0x41, 0xd3, 0x74, 0x19, 0x34, 0x4f, 0x8f, 0xd1, 0x20,
      0x71, 0x9a, 0x4e, 0x07, 0x16, 0x01, 0x00, 0x00};
  uint8_t example_pkt6[] = {
      0x01, 0x3a, 0x00, 0x80, 0x05, 0x01, 0x04, 0x07, 0x00, 0x01, 0x00, 0x2e,
      0x00, 0x11, 0x27, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x1f, 0x00,
      0x40, 0x00, 0x11, 0x1c, 0x00, 0x66, 0x65, 0x50, 0xd8, 0x0d, 0x4a, 0xa2,
      0xe5, 0x65, 0x37, 0xe8, 0x58, 0x96, 0xef, 0xe9, 0x65, 0x74, 0x59, 0x3e,
      0xa7, 0x97, 0xd9, 0xec, 0xb2, 0xdc, 0xd5, 0x16, 0x01, 0x00, 0x00};
  strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "Dummy CB Message parse\n");
  add_message_to_queue(reply, strsz);
  check_cb_message(example_pkt1, sizeof(example_pkt1), 0, 0);
  check_cb_message(example_pkt2, sizeof(example_pkt2), 0, 0);
  check_cb_message(example_pkt3, sizeof(example_pkt3), 0, 0);
  check_cb_message(example_pkt4, sizeof(example_pkt4), 0, 0);
  check_cb_message(example_pkt5, sizeof(example_pkt5), 0, 0);
  check_cb_message(example_pkt6, sizeof(example_pkt6), 0, 0);
  free(reply);
  reply = NULL;
}

void debug_ucs2_cb_message(uint8_t *command) {
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  uint8_t pkt1[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x03, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x63, 0x40, 0x00, 0x32, 0x59, 0x14, 0x00, 0x20, 0x04, 0x1f, 0x04, 0x40,
      0x04, 0x3e, 0x04, 0x42, 0x04, 0x4f, 0x04, 0x33, 0x04, 0x3e, 0x04, 0x3c,
      0x00, 0x20, 0x04, 0x34, 0x04, 0x3d, 0x04, 0x4f, 0x00, 0x20, 0x04, 0x54,
      0x00, 0x20, 0x04, 0x32, 0x04, 0x38, 0x04, 0x41, 0x04, 0x3e, 0x04, 0x3a,
      0x04, 0x30, 0x00, 0x20, 0x04, 0x56, 0x04, 0x3c, 0x04, 0x3e, 0x04, 0x32,
      0x04, 0x56, 0x04, 0x40, 0x04, 0x3d, 0x04, 0x56, 0x04, 0x41, 0x04, 0x42,
      0x04, 0x4c, 0x00, 0x20, 0x04, 0x40, 0x04, 0x30, 0x04, 0x3a, 0x04, 0x35,
      0x04, 0x42, 0x04, 0x3d, 0x16, 0x01, 0x00, 0x00};
  uint8_t pkt2[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x04, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x63, 0x40, 0x00, 0x32, 0x59, 0x24, 0x04, 0x38, 0x04, 0x45, 0x00, 0x20,
      0x04, 0x43, 0x04, 0x34, 0x04, 0x30, 0x04, 0x40, 0x04, 0x56, 0x04, 0x32,
      0x00, 0x20, 0x04, 0x3f, 0x04, 0x3e, 0x00, 0x20, 0x04, 0x42, 0x04, 0x35,
      0x04, 0x40, 0x04, 0x38, 0x04, 0x42, 0x04, 0x3e, 0x04, 0x40, 0x04, 0x56,
      0x04, 0x57, 0x00, 0x20, 0x04, 0x23, 0x04, 0x3a, 0x04, 0x40, 0x04, 0x30,
      0x04, 0x57, 0x04, 0x3d, 0x04, 0x38, 0x00, 0x2e, 0x00, 0x20, 0x04, 0x17,
      0x04, 0x30, 0x04, 0x3b, 0x04, 0x38, 0x04, 0x48, 0x04, 0x30, 0x04, 0x39,
      0x04, 0x42, 0x04, 0x35, 0x16, 0x01, 0x00, 0x00};
  uint8_t pkt3[] = {
      0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x67,
      0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00,
      0x63, 0x40, 0x00, 0x32, 0x59, 0x34, 0x04, 0x41, 0x04, 0x4c, 0x00, 0x20,
      0x04, 0x32, 0x00, 0x20, 0x04, 0x43, 0x04, 0x3a, 0x04, 0x40, 0x04, 0x38,
      0x04, 0x42, 0x04, 0x42, 0x04, 0x4f, 0x04, 0x45, 0x00, 0x20, 0x04, 0x37,
      0x04, 0x30, 0x04, 0x40, 0x04, 0x30, 0x04, 0x34, 0x04, 0x38, 0x00, 0x20,
      0x04, 0x12, 0x04, 0x30, 0x04, 0x48, 0x04, 0x3e, 0x04, 0x57, 0x00, 0x20,
      0x04, 0x31, 0x04, 0x35, 0x04, 0x37, 0x04, 0x3f, 0x04, 0x35, 0x04, 0x3a,
      0x04, 0x38, 0x00, 0x2e, 0x00, 0x20, 0x04, 0x1d, 0x04, 0x35, 0x00, 0x20,
      0x04, 0x3d, 0x04, 0x35, 0x16, 0x01, 0x00, 0x00};
  uint8_t pkt4[] = {
      0x01, 0x72, 0x00, 0x80, 0x05, 0x01, 0x04, 0x06, 0x00, 0x01, 0x00, 0x66,
      0x00, 0x11, 0x5f, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x57, 0x00,
      0x63, 0x40, 0x00, 0x32, 0x59, 0x44, 0x04, 0x45, 0x04, 0x42, 0x04, 0x43,
      0x04, 0x39, 0x04, 0x42, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x38,
      0x04, 0x33, 0x04, 0x3d, 0x04, 0x30, 0x04, 0x3b, 0x04, 0x30, 0x04, 0x3c,
      0x04, 0x38, 0x00, 0x20, 0x00, 0x22, 0x04, 0x1f, 0x04, 0x3e, 0x04, 0x32,
      0x04, 0x56, 0x04, 0x42, 0x04, 0x40, 0x04, 0x4f, 0x04, 0x3d, 0x04, 0x30,
      0x00, 0x20, 0x04, 0x42, 0x04, 0x40, 0x04, 0x38, 0x04, 0x32, 0x04, 0x3e,
      0x04, 0x33, 0x04, 0x30, 0x00, 0x22, 0x00, 0x2e, 0x00, 0x0d, 0x00, 0x0d,
      0x00, 0x0d, 0x00, 0x16, 0x01, 0x00, 0x00};

  check_cb_message(pkt1, sizeof(pkt1), 0, 0);
  check_cb_message(pkt2, sizeof(pkt2), 0, 0);
  check_cb_message(pkt3, sizeof(pkt3), 0, 0);
  check_cb_message(pkt4, sizeof(pkt4), 0, 0);

  free(reply);
  reply = NULL;
}

void dump_signal_report() {
  int strsz = 0;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  struct nas_report report = get_current_cell_report();
  if (report.mcc == 0) {
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Serving cell report has not been retrieved yet or is invalid\n");
    add_message_to_queue(reply, strsz);
    free(reply);
    reply = NULL;
    return;
  }

  strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "MCC: %i MNC: %i\n",
                   report.mcc, report.mnc);
  switch (report.type_of_service) {
  case OCID_RADIO_GSM:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Current Network: GSM\n");
    break;
  case OCID_RADIO_UMTS:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Current Network: UMTS\n");
    break;
  case OCID_RADIO_LTE:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Current Network: LTE\n");
    break;
  case OCID_RADIO_NR:
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "Current Network: 5G\n");
    break;
  }
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Cell: %.8x\n", report.cell_id);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Location Area Code: %.4x\n", report.lac);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "(E/)ARFCN %u\n", report.arfcn);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "SRX Level (min) %i dBm\n", report.srx_level_min);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "SRX Level (max) %i dBm\n", report.srx_level_max);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "RX Level (min) %u\n", report.rx_level_min);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "RX Level (max) %u\n", report.rx_level_max);
  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "PSC (umts) %.4x\n", report.psc);
  strsz +=
      snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "Verified? %s",
               report.opencellid_verified == 1 ? "Yes" : "No");
  add_message_to_queue(reply, strsz);
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
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  logger(MSG_WARN, "SCH: %s -> %s \n", cmd, command);

  int delaysec;
  char tmpbuf[10];
  char *secoffset;
  offset = (uint8_t *)strstr(
      (char *)command, bot_commands[CMD_ID_ACTION_CALL_OWNER_COUNTDOWN].cmd);
  if (offset == NULL) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE - strsz,
                     "Error reading the command\n");
    add_message_to_queue(reply, strsz);
  } else {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_CALL_OWNER_COUNTDOWN].cmd);
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
          get_rt_user_name());
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
  char *network_types[] = {"Unknown", "CDMA",  "EVDO",  "AMPS", "GSM",
                           "UMTS",    "Error", "Error", "LTE"};

  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
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
                    "Signal strength: -%i dBm\n", (int)get_signal_strength());
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
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
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
  offset_command = (uint8_t *)strstr(
      (char *)command, bot_commands[CMD_ID_ACTION_SET_REMINDER].cmd);
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
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
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
  offset_command = (uint8_t *)strstr(
      (char *)command, bot_commands[CMD_ID_ACTION_WAKE_OWNER].cmd);
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
           "It's time to wakeup, %s", get_rt_user_name());
  if (add_task(scheduler_task) < 0) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     " Can't schedule wakeup, my task queue is full!\n");
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

void suspend_call_notifications(uint8_t *command) {
  uint8_t *offset_command;
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  int strsz = 0;
  char temp_str[160];
  char current_word[160] = {0};
  int markers[128] = {0};
  int phrase_size = 1;
  int start = 0;
  int end = 0;
  char sep[] = " ";
  struct task_p scheduler_task;
  scheduler_task.time.mode = SCHED_MODE_TIME_COUNTDOWN; // 0 at, 1 in
  /* Initial command check */
  offset_command = (uint8_t *)strstr(
      (char *)command, bot_commands[CMD_ID_ACTION_ENABLE_DND].cmd);
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

  logger(MSG_DEBUG, "%s: Total words in command: %i\n", __func__, phrase_size);
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
     * leave me alone for 00[:00]
     *  ^    ^  ^      ^  ^^  ^^
     *  0    1  2      3   4  5OPT
     */

    switch (i) {
    case 0: /* Fall through */
    case 1:
    case 2:
      logger(MSG_DEBUG, "Current word in pos %i: %s\n", i, current_word);
      break;
    case 3:
      if (strchr(current_word, ':') != NULL) {
        logger(MSG_DEBUG, "%s: Time has minutes", __func__);
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
        } else {
          strsz =
              snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Blocking incoming calls for %i hours and %i minutes\n",
                       scheduler_task.time.hh, scheduler_task.time.mm);
        }
      } else {
        logger(MSG_INFO, "%s:  Only hours have been specified\n", __func__);
        if (strlen(current_word) > 2) {
          logger(MSG_WARN, "%s: How long do you want me to wait? %s\n",
                 __func__, current_word);
          strsz =
              snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "I can't keep DND on _that_ long... %s\n", current_word);
          add_message_to_queue(reply, strsz);
          free(reply);
          reply = NULL;
          return;
        } else {
          scheduler_task.time.hh = atoi(current_word);
          scheduler_task.time.mm = 0;
          if (scheduler_task.time.hh > 0) {
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Blocking incoming calls for %i hour(s)\n",
                             scheduler_task.time.hh);

          } else {
            strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "%i hours is not a valid time! Please specify the "
                             "number of hours\n",
                             scheduler_task.time.hh);
            add_message_to_queue(reply, strsz);
            free(reply);
            reply = NULL;
            return;
          }
        }
      }
      break;
    }
  }

  set_do_not_disturb(true);
  scheduler_task.type = TASK_TYPE_DND_CLEAR;
  scheduler_task.status = STATUS_PENDING;
  scheduler_task.param = 0;
  scheduler_task.time.mode = scheduler_task.time.mode;
  snprintf(scheduler_task.arguments, MAX_MESSAGE_SIZE,
           "Hi %s, as requested, I'm disabling DND now!", get_rt_user_name());
  remove_all_tasks_by_type(
      TASK_TYPE_DND_CLEAR); // We clear all previous timers set
  if (add_task(scheduler_task) < 0) {
    set_do_not_disturb(false);
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Can't schedule DND disable, so keeping it off! (too many tasks)\n");
  }
  add_message_to_queue(reply, strsz);
  free(reply);
  reply = NULL;
}

int find_dictionary_entry(char *word) {
  char *line = malloc(32768); /* initialize all to 0 ('\0') */
  bool found = false;
  static const char *wtypes[] = {"noun",         "preposition", "adjective",
                                 "verb",         "adverb",      "pronoun",
                                 "interjection", "conjunction", "pronoun"};
  FILE *dictfile;
  uint32_t id = 0;
  uint8_t type = 0;
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  size_t def_size = 0;
  dictfile = fopen(DICT_PATH, "r");
  if (dictfile == NULL) {
    logger(MSG_INFO, "Failed to open dictionary file!\n");
    free(line);
    return -ENOMEM;
  }

  fseek(dictfile, 0, SEEK_SET);

  /* Loop through the dictionary until we find the word */
  while (fgets(line, 32768, dictfile) && !found) {
    int count = 0;
    char *next = strtok(line, ":");
    found = 0;
    // ID:TYPE:WORD:DEFINITION\n
    while (next != NULL) {
      switch (count) {
      case 0:
        id = atoi(next);
        break;
      case 1:
        type = atoi(next);
        break;
      case 2:
        if (strcmp(next, word) == 0) {
          logger(MSG_DEBUG, "--> Word found: %s -> %s (id %u type %u [%s])\n",
                 word, next, id, type, wtypes[type]);
          found = 1;
        }
        break;
      case 3:
        if (found) {
          logger(MSG_DEBUG, "--> Definition: %s: %s\n", next, wtypes[type]);
          def_size = strlen(next);
          while (strsz < def_size) {
            memset(reply, 0, MAX_MESSAGE_SIZE);
            if (def_size - strsz > MAX_MESSAGE_SIZE) {
              strncpy((char *)reply, (next + strsz), MAX_MESSAGE_SIZE - 1);
              add_message_to_queue(reply, MAX_MESSAGE_SIZE);
              strsz += MAX_MESSAGE_SIZE - 1;
            } else {
              strncpy((char *)reply, (next + strsz), (def_size - strsz));
              add_message_to_queue(reply, (def_size - strsz));
              strsz += (def_size - strsz);
            }
          }
          free(line);
          fclose(dictfile);
          return 0;
        }
      }
      next = strtok(NULL, ":");
      count++;
    };
  }
  logger(MSG_DEBUG, "--> Word '%s' not found\n", word);
  memset(reply, 0, MAX_MESSAGE_SIZE);
  strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                   "I don't know what '%s' means :(\n", word);
  add_message_to_queue(reply, strsz);
  free(line);
  fclose(dictfile);

  return 0;
}

void search_dictionary_entry(uint8_t *command) {
  uint8_t *offset;
  char word[128];
  offset = (uint8_t *)strstr((char *)command, bot_commands[CMD_ID_DEFINE].cmd);
  if (offset != NULL) {
    int ofs = (int)(offset - command) + strlen(bot_commands[CMD_ID_DEFINE].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(word, 128, "%s", (char *)command + ofs);
      find_dictionary_entry(word);
    }
  }
}

void configure_new_apn(uint8_t *command) {
  uint8_t *offset;
  char apn[128];
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  offset = (uint8_t *)strstr(
      (char *)command,
      bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_APN].cmd);
  if (offset != NULL) {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_APN].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(apn, 128, "%s", (char *)command + ofs);
      set_internal_network_apn_name(apn);
      // Send notif
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Setting internal APN name to %s\n", apn);
      add_message_to_queue(reply, strsz);
    }
  } else {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Error setting internal APN name \n");
    add_message_to_queue(reply, strsz);
  }
}

void configure_apn_username(uint8_t *command) {
  uint8_t *offset;
  char user[128];
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  offset = (uint8_t *)strstr(
      (char *)command,
      bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_USER].cmd);
  if (offset != NULL) {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_USER].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(user, 128, "%s", (char *)command + ofs);
      set_internal_network_username(user);
      // Send notif
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Setting internal APN username to %s\n", user);
      add_message_to_queue(reply, strsz);
    }
  } else {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Error setting APN username \n");
    add_message_to_queue(reply, strsz);
  }
}

void configure_apn_password(uint8_t *command) {
  uint8_t *offset;
  char pass[128];
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  offset = (uint8_t *)strstr(
      (char *)command,
      bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_PASS].cmd);
  if (offset != NULL) {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_PASS].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(pass, 128, "%s", (char *)command + ofs);
      set_internal_network_username(pass);
      // Send notif
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Setting internal APN password to %s\n", pass);
      add_message_to_queue(reply, strsz);
    }
  } else {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Error setting APN password \n");
    add_message_to_queue(reply, strsz);
  }
}

void configure_internal_network_auth_method(uint8_t *command) {
  uint8_t *offset;
  uint8_t changed = 0;
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  char word[128];
  offset = (uint8_t *)strstr(
      (char *)command,
      bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_AUTH_METHOD].cmd);
  if (offset != NULL) {
    int ofs =
        (int)(offset - command) +
        strlen(
            bot_commands[CMD_ID_ACTION_INTERNAL_NETWORK_SET_AUTH_METHOD].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(word, 128, "%s", (char *)command + ofs);
      if (strcmp(word, "none") == 0) {
        changed = 1;
        set_internal_network_auth_method(0);
      } else if (strcmp(word, "pap") == 0) {
        changed = 1;
        set_internal_network_auth_method(1);
      } else if (strcmp(word, "chap") == 0) {
        changed = 1;
        set_internal_network_auth_method(2);
      } else if (strcmp(word, "auto") == 0) {
        set_internal_network_auth_method(2);
        changed = 1;
      }
    }
  }
  if (changed) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Auth mode changed: %s \n", word);
  } else {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "Unknown auth mode!\n");
  }
  add_message_to_queue(reply, strsz);
}

void configure_signal_tracking_cell_notification(uint8_t *command) {
  uint8_t *offset;
  uint8_t changed = 0;
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  char word[128];
  offset = (uint8_t *)strstr(
      (char *)command,
      bot_commands[CMD_ID_ACTION_SET_ST_NOTIFICATION_LEVEL].cmd);
  if (offset != NULL) {
    int ofs = (int)(offset - command) +
              strlen(bot_commands[CMD_ID_ACTION_SET_ST_NOTIFICATION_LEVEL].cmd);
    if (strlen((char *)command) > ofs) {
      snprintf(word, 128, "%s", (char *)command + ofs);
      if (strcmp(word, "none") == 0) {
        changed = 1;
        set_signal_tracking_cell_change_notification(0);
      } else if (strcmp(word, "new") == 0) {
        changed = 1;
        set_signal_tracking_cell_change_notification(1);
      } else if (strcmp(word, "all") == 0) {
        changed = 1;
        set_signal_tracking_cell_change_notification(2);
      }
    }
  }
  if (changed) {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Cell ID change notificaitons: %s \n", word);
  } else {
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                     "Unknown Cell ID change notification mode!\n");
  }
  add_message_to_queue(reply, strsz);
}

void clear_internal_networking_auth() {
  size_t strsz;
  uint8_t reply[MAX_MESSAGE_SIZE];
  set_internal_network_auth_method(0);
  set_internal_network_username("");
  set_internal_network_pass("");
  set_internal_network_apn_name("");
  strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "APN Auth data cleared\n");
  add_message_to_queue(reply, strsz);
}

void set_cb_broadcast(bool en) {
  char *response = malloc(128 * sizeof(char));
  uint8_t *reply = calloc(MAX_MESSAGE_SIZE, sizeof(unsigned char));
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

void cmd_get_openqti_log() {
  size_t strsz = 0;
  int ret = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  FILE *fp = fopen(get_openqti_logfile(), "r");
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
}
void cmd_get_kernel_log() {
  size_t strsz = 0;
  int ret = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  FILE *fp = fopen("/var/log/messages", "r");
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
}

void cmd_get_sw_version() {
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  strsz +=
      snprintf((char *)reply, MAX_MESSAGE_SIZE,
               "FW Ver:%s\n"
               "ADSP Ver:%s\n"
               "Serial:%s\n"
               "HW Rev:%s\n"
               "Rev:%s\n"
               "Model:%s\n",
               RELEASE_VER, known_adsp_fw[read_adsp_version()].fwstring,
               dms_get_modem_modem_serial_num(), dms_get_modem_modem_hw_rev(),
               dms_get_modem_revision(), dms_get_modem_modem_model());
  if (strsz > 159) {
    strsz = 159;
  }
  add_message_to_queue(reply, strsz);
}

void cmd_thank_you() {
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  FILE *fp;
  int ret = 0;

  strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                    "Hi, my name is %s and I'm at version %s\n",
                    get_rt_modem_name(), RELEASE_VER);
  add_message_to_queue(reply, strsz);
  strsz = snprintf(
      (char *)reply, MAX_MESSAGE_SIZE,
      "    .-.     .-.\n .****. .****.\n.*****.*****. Thank\n  .*********.   "
      " You\n    .*******.\n     .*****.\n       .***.\n          *\n");
  add_message_to_queue(reply, strsz);
  strsz =
      snprintf((char *)reply, MAX_MESSAGE_SIZE,
               "Thank you for using me!\n And, especially, for all of you...");
  add_message_to_queue(reply, strsz);
  fp = fopen("/usr/share/thank_you/thankyou.txt", "r");
  if (fp != NULL) {
    size_t len = 0;
    char *line;
    memset(reply, 0, MAX_MESSAGE_SIZE);
    strsz = 0;
    uint16_t i = 0;
    while ((ret = getline(&line, &len, fp)) != -1) {
      strsz +=
          snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s", line);
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
}