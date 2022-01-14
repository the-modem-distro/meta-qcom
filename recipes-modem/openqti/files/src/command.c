// SPDX-License-Identifier: MIT

#include "../inc/command.h"
#include "../inc/logger.h"
#include "../inc/proxy.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
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
  uint32_t last_cmd_timestamp;
  char user_name[32];
  char bot_name[32];
} cmd_runtime;

void set_cmd_runtime_defaults() {
  cmd_runtime.is_unlocked = false;
  cmd_runtime.unlock_time = 0;
  strncpy(cmd_runtime.user_name, "User", 32); //FIXME: Allow user to set a custom name
  strncpy(cmd_runtime.bot_name, "Modem", 32); //FIXME: Allow to change modem name
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

  bytes_written = snprintf((char *)output, 256, "%02u:%02u:%02u up ", current_time->tm_hour,
           current_time->tm_min, current_time->tm_sec);
  updays = (unsigned)info.uptime / (unsigned)(60 * 60 * 24);
  if (updays)
    bytes_written += snprintf((char *)output + bytes_written, 256 - bytes_written, "%u day%s, ", updays,
             (updays != 1) ? "s" : "");
  upminutes = (unsigned)info.uptime / (unsigned)60;
  uphours = (upminutes / (unsigned)60) % (unsigned)24;
  upminutes %= 60;
  if (uphours)
    bytes_written += snprintf((char *)output + bytes_written, 256 - bytes_written, "%2u:%02u", uphours, upminutes);
  else
    bytes_written += snprintf((char *)output + bytes_written, 256 - bytes_written, "%u min", upminutes);

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

  snprintf((char *)output, 256, "Total:%luM\nFree:%luM\nShared:%luK\nBuffer:%luK\nProcs:%i\n",
           (info.totalram / 1024/ 1024),
           (info.freeram / 1024/ 1024),
           (info.sharedram / 1024),
           (info.bufferram / 1024),
           info.procs
           );

  return 0;
}

uint8_t parse_command(uint8_t *command, uint8_t *reply) {
  int ret = 0;
  int cmd_id = -1;
  int strcount = 0;
  struct pkt_stats packet_stats;
  uint8_t *tmpbuf = calloc(128, sizeof(unsigned char));
  set_cmd_runtime_defaults();
  for (int i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0])); i++) {
    if (strcmp((char*)command, bot_commands[i].cmd) == 0) {
      cmd_id = bot_commands[i].id;
    }
  }

  switch (cmd_id) {
  case -1:
    logger(MSG_INFO, "%s: Nothing to do\n", __func__);
    strcount = snprintf((char *)reply, 256, "Command not found %s!\n", command);
    break;
  case 0:
    strcount =
        snprintf((char *)reply, 256, "My name is %s\n", cmd_runtime.bot_name);
    break;
  case 1:
    if (get_uptime(tmpbuf) == 0) {
      strcount = snprintf((char *)reply, 256,
                          "This is my uptime:\n %s\n", tmpbuf);
    } else {
      strcount = snprintf((char *)reply, 256, "Error getting the uptime\n");
    }
    break;
  case 2:
    if (get_load_avg(tmpbuf) == 0) {
      strcount = snprintf((char *)reply, 256, "My current load avg is: %s\n", tmpbuf);
    } else {
      strcount = snprintf((char *)reply, 256, "Error getting laodavg\n");
    }
    break;
  case 3:
    strcount = snprintf((char *)reply, 256, "I'm at version %s\n", RELEASE_VER);
    break;
  case 4:
    strcount = snprintf((char *)reply, 256, "USB Suspend state: %i\n",
                        get_transceiver_suspend_state());
    break;
  case 5:
    if (get_memory(tmpbuf) == 0) {
      strcount = snprintf((char *)reply, 256, "Memory stats:\n%s\n", tmpbuf);
    } else {
      strcount = snprintf((char *)reply, 256, "Error getting laodavg\n");
    }
    break;
  case 6:
    packet_stats = get_rmnet_stats();
    strcount = snprintf((char *)reply, 256, "RMNET IF stats:\nBypassed: %i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%i",
    packet_stats.bypassed, packet_stats.empty, packet_stats.discarded, packet_stats.failed, packet_stats.allowed);
    break;
  case 7:
    packet_stats = get_gps_stats();
    strcount = snprintf((char *)reply, 256, "GPS IF stats:\nBypassed: %i\nEmpty:%i\nDiscarded:%i\nFailed:%i\nAllowed:%i\nQMI Location svc.: %i",
    packet_stats.bypassed, packet_stats.empty, packet_stats.discarded, packet_stats.failed, packet_stats.allowed, packet_stats.other);
    break;
  case 8:
    strcount = snprintf((char *)reply, 256, "Commands:\nhelp\nname\nuptime\nload\nversion\nmemory\nnet stats\ngps stats\ncaffeinate\ndecaff\nenable adb\ndisable adb\n");
    break;
  case 9:
    strcount = snprintf((char *)reply, 256, "Blocking USB suspend until reboot or until you tell me otherwise!\n");
    set_suspend_inhibit(false);
    break;
  case 10:
    strcount = snprintf((char *)reply, 256, "Allowing USB tu suspend again\n");
    set_suspend_inhibit(false);
    break;
  case 11:
    strcount = snprintf((char *)reply, 256, "Turning ADB *ON*\n");
    store_adb_setting(true);
    restart_usb_stack();   
    break;
  case 12:
    strcount = snprintf((char *)reply, 256, "Turning ADB *OFF*\n");
    store_adb_setting(false);
    restart_usb_stack();
    break;

  default:
    strcount = snprintf((char *)reply, 256, "Invalid command id %i\n", cmd_id);
    logger(MSG_INFO, "%s: Unknown command %i\n", __func__, cmd_id);
    break;
  }

  free(tmpbuf);
  tmpbuf = NULL;
  return ret;
}

