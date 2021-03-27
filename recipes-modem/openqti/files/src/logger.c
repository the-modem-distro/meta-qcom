// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "../inc/openqti.h"

void logger(bool debug_to_stdout, uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  if (debug_to_stdout) {
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


void dump_packet(bool debug_to_stdout,char *direction, char *buf) {
  int i;
  FILE *fd;
  if (debug_to_stdout) {
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
