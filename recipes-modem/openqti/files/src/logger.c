// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "../inc/openqti.h"

uint8_t log_level = 0;
bool log_to_file = true;
void set_log_method(bool ttyout) {
  if (ttyout) {
    log_to_file = false;
  } else {
    log_to_file = true;
  }
}
void set_log_level(uint8_t level) {
  if (level >= 0 && level <= 2) {
    log_level = level;
  }
}
void logger(uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  if (level >= log_level) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      fd = fopen("/var/log/openqti.log", "a");
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }
    switch (level) {
    case 0:
      fprintf(fd, "[DBG]");
      break;
    case 1:
      fprintf(fd, "[WRN]");
      break;
    default:
      fprintf(fd, "[ERR]");
      break;
    }
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);
    fflush(fd);
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

void dump_packet(char *direction, char *buf) {
  int i;
  FILE *fd;
  if (log_level == 0) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      fd = fopen("/var/log/openqti.log", "a");
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }
    fprintf(fd, "%s :", direction);
    for (i = 0; i < sizeof(buf); i++) {
      fprintf(fd, "0x%02x ", buf[i]);
    }
    fprintf(fd, "\n");
    if (fd != stdout) {
      fclose(fd);
    }
  }
}
