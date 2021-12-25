// SPDX-License-Identifier: MIT

#include "timesync.h"
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

uint32_t year, month, day, hour, minute, second;

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
  fd = open(SMD_DEVICE_PATH, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "%s: Cannot open SMD10 entry\n", __func__);
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
    year = get_int_from_str(begin, 1);
    month = get_int_from_str(begin, 4);
    day = get_int_from_str(begin, 7);
    hour = get_int_from_str(begin, 10);
    minute = get_int_from_str(begin, 13);
    second = get_int_from_str(begin, 16);
    if (year == 80) {
      year = 0;
    }
  }
  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  FILE *fd;
  fprintf(stdout, "Time sync!\n");
  year = 0;
  time_t mytime = time(0);
  struct tm *tm_ptr = localtime(&mytime);
  while (year < 21) {
    sleep(60);
    get_carrier_datetime();
    fprintf(stdout, "Year is %i/%i/%i %i:%i:%i\n", year, month, day, hour,
            minute, second);
    if (year < 21) {
      fprintf(stderr, "Time travel hasn't been invented yet! \n");
    } else {
      if (tm_ptr) {
        tm_ptr->tm_mon = month - 1;
        tm_ptr->tm_mday = day;
        tm_ptr->tm_year = year + 100;
        tm_ptr->tm_hour = hour;
        tm_ptr->tm_min = minute;
        tm_ptr->tm_sec = second;
        const struct timeval tv = {mktime(tm_ptr), 0};
        fprintf(stdout, "Trying to set the time\n");
        settimeofday(&tv, 0);
      }
    }
  }

  return 0;
}
