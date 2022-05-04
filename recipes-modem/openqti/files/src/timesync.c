// SPDX-License-Identifier: MIT

#include "../inc/timesync.h"
#include "../inc/cell.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/*
 * Timesync
 *    Ugly utility to read the current time from the ADSP
 *    and sync the local RTC with it
 */

struct {
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;
  int timezone_offset;
  bool negative_offset;
} time_sync_data;

int get_timezone() { return time_sync_data.timezone_offset; }

bool is_timezone_offset_negative() { return time_sync_data.negative_offset; }

int send_at_command(char *at_command, size_t cmdlen, char *response) {
  int fd, ret;
  fd = open(SMD_SEC_AT, O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open %s\n", __func__, SMD_SEC_AT);
    return -EINVAL;
  }
  ret = write(fd, at_command, cmdlen);
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Failed to write to %s\n", __func__, SMD_SEC_AT);
    return -EIO;
  }
  sleep(1);
  ret = read(fd, response, 128);
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Failed to read from %s\n", __func__, SMD_SEC_AT);
    return -EIO;
  }
  close(fd);
  return 0;
}

void *time_sync() {
  int cmd_ret;
  char *begin;
  int tmp;
  long gmtoff;
  bool sync_completed = false;
  char *response = malloc(128 * sizeof(char));
  time_t mytime = time(0);
  struct tm *tm_ptr = localtime(&mytime);
  logger(MSG_INFO, "%s: Time Sync thread starting... \n", __func__);
  /* Lock the thread until we get a signal fix */
  while (!get_network_type()) {
    logger(MSG_INFO, "%s: Waiting for network to be ready... %i\n", __func__,
           is_network_in_service());
    sleep(30);
  }

  while (!sync_completed) {
    memset(response, 0, 128);
    // Set CTZU first
    logger(MSG_INFO, "%s: Send CTZU\n", __func__);
    cmd_ret = send_at_command(SET_CTZU, sizeof(SET_CTZU), response);
    sleep(1);
    // Now attempt to sync from network
    logger(MSG_INFO, "%s: Send QLTS\n", __func__);
    cmd_ret = send_at_command(GET_QLTS, sizeof(GET_QLTS), response);
    if (strstr(response, "+QLTS: ") != NULL) { // Sync was ok
      begin = strchr(response, '"');
      if (get_int_from_str(begin, 1) == 20 &&
          get_int_from_str(begin, 3) >= 22) {
        time_sync_data.year = get_int_from_str(begin, 3);
        time_sync_data.month = get_int_from_str(begin, 6);
        time_sync_data.day = get_int_from_str(begin, 9);
        time_sync_data.hour = get_int_from_str(begin, 12);
        time_sync_data.minute = get_int_from_str(begin, 15);
        time_sync_data.second = get_int_from_str(begin, 18);
        time_sync_data.timezone_offset = get_int_from_str(begin, 21);
        if (time_sync_data.timezone_offset != 0) {
          time_sync_data.timezone_offset = time_sync_data.timezone_offset / 4;
          if (begin[20] == '-') {
            time_sync_data.negative_offset = true;
          }
        }
        sync_completed = true; // Might need to recheck here, can't recall the
                               // types of invalid reported dates by carriers...
        logger(MSG_INFO,
               "%s: Network reported time: %i-%.2i-%.2i %.2i:%.2i:%.2i\n",
               __func__, time_sync_data.year, time_sync_data.month,
               time_sync_data.day, time_sync_data.hour, time_sync_data.minute,
               time_sync_data.second);
      }
    } else {
      logger(MSG_WARN,
             "%s: Couldn't sync time from the network, attempting local sync\n",
             __func__);
      logger(MSG_INFO, "%s: Send CCLK\n", __func__);
      cmd_ret = send_at_command(GET_CCLK, sizeof(GET_CCLK), response);
      if (strstr(response, "+CCLK: ") != NULL) {
        begin = strchr(response, '"');
        if (begin != NULL) {
          tmp = get_int_from_str(begin, 1);
          if (tmp != 80) { // Year
            // year
            time_sync_data.year = get_int_from_str(begin, 1);
            time_sync_data.month = get_int_from_str(begin, 4);
            time_sync_data.day = get_int_from_str(begin, 7);
            time_sync_data.hour = get_int_from_str(begin, 10);
            time_sync_data.minute = get_int_from_str(begin, 13);
            time_sync_data.second = get_int_from_str(begin, 16);
            time_sync_data.timezone_offset = get_int_from_str(begin, 19);
            /* Time zone is reported in 15 minute blocks */
            if (time_sync_data.timezone_offset != 0) {
              time_sync_data.timezone_offset =
                  time_sync_data.timezone_offset / 4;
              if (begin[18] == '-') {
                time_sync_data.negative_offset = true;
              }
            }
            sync_completed = true;
          } // if year != 80
        }
      }
    }
    if (time_sync_data.month > 12 || time_sync_data.day > 31 ||
        time_sync_data.hour > 23 || time_sync_data.minute > 59) {
      sync_completed = false;
      logger(MSG_WARN, "%s: Invalid date, retrying sync...\n", __func__);
    }
  }
  if (time_sync_data.timezone_offset != 0) {
    // gmtoff is specified in seconds
    gmtoff = time_sync_data.timezone_offset * 60 * 60;
    if (time_sync_data.negative_offset) {
      gmtoff *= -1;
    }
  }
  logger(MSG_INFO,
         "%s: Syncing date and time: %i-%i-%i %.2i:%.2i:%.2i (GMT %ld)\n",
         __func__, time_sync_data.year, time_sync_data.month,
         time_sync_data.day, time_sync_data.hour, time_sync_data.minute,
         time_sync_data.second, (gmtoff / 3600));
  if (tm_ptr) {
    tm_ptr->tm_mon = time_sync_data.month - 1;
    tm_ptr->tm_mday = time_sync_data.day;
    tm_ptr->tm_year = time_sync_data.year + 100;
    tm_ptr->tm_hour = time_sync_data.hour;
    tm_ptr->tm_min = time_sync_data.minute;
    tm_ptr->tm_sec = time_sync_data.second;
    if (time_sync_data.timezone_offset != 0) {
      // gmtoff is specified in seconds
      gmtoff = time_sync_data.timezone_offset * 60 * 60;
      if (time_sync_data.negative_offset) {
        gmtoff *= -1;
      }
    }
  }
  const struct timeval tv = {(mktime(tm_ptr) + gmtoff), 0};
  settimeofday(&tv, 0);
  free(response);
  return NULL;
}
