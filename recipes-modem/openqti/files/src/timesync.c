// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "../inc/devices.h"
#include "../inc/logger.h"
#include "../inc/timesync.h"


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

int get_timezone() {
  return time_sync_data.timezone_offset;
}

bool is_timezone_offset_negative() {
  return time_sync_data.negative_offset;
}

int get_int_from_str(char *str, int offset) {
  int val = 0;
  char tmp[2];
  tmp[0] = str[offset];
  tmp[1] = str[offset + 1];
  val = strtol(tmp, NULL, 10);
  return val;
}

int get_carrier_datetime() {
  int fd, ret;
  char response[128];
  char *begin;
  fd = open(SMD_SEC_AT, O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open SMD10 entry\n", __func__);
    return -EINVAL;
  }
  ret = write(fd, SET_CTZU, sizeof(SET_CTZU));
  sleep(1);
  ret = read(fd, &response, 128);
  ret = write(fd, GET_CCLK, sizeof(GET_CCLK));
  sleep(1);
  ret = read(fd, &response, 128);
  if (strstr(response, "+CCLK: ") != NULL) {
    begin = strchr(response, '"');
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
      time_sync_data.timezone_offset = time_sync_data.timezone_offset/4;
      if (begin[18] == '-') {
        time_sync_data.negative_offset = true;
      }

    }
    if (time_sync_data.year == 80) {
      time_sync_data.year = 0;
    }
  }
  close(fd);
  return 0;
}

void *time_sync() {
  FILE *fd;
  logger(MSG_INFO, "%s: Time Sync thread starting... \n", __func__);
  time_sync_data.year = 0;
  time_sync_data.timezone_offset = 0;
  time_sync_data.negative_offset = false;
  time_t mytime = time(0);
  struct tm *tm_ptr = localtime(&mytime);
  while (time_sync_data.year < 21) {
    sleep(60);
    get_carrier_datetime();
    logger(MSG_INFO, "%s: Current date is %i/%i/%i %i:%i:%i\n",
                      __func__, 
                      time_sync_data.year, 
                      time_sync_data.month, 
                      time_sync_data.day, 
                      time_sync_data.hour,
                      time_sync_data.minute, 
                      time_sync_data.second);
    if (time_sync_data.year < 21) {
      logger(MSG_WARN, "%s: Time travel hasn't been invented yet... Retrying time sync \n", __func__);
    } else {
      if (tm_ptr) {
        tm_ptr->tm_mon = time_sync_data.month - 1;
        tm_ptr->tm_mday = time_sync_data.day;
        tm_ptr->tm_year = time_sync_data.year + 100;
        tm_ptr->tm_hour = time_sync_data.hour;
        tm_ptr->tm_min = time_sync_data.minute;
        tm_ptr->tm_sec = time_sync_data.second;
        const struct timeval tv = {mktime(tm_ptr), 0};
        logger(MSG_INFO, "%s: Syncing time\n", __func__);
        settimeofday(&tv, 0);
      }
    }
  }

  return NULL;
}
