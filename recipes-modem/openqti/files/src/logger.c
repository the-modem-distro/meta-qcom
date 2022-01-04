// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "../inc/helpers.h"
#include "../inc/openqti.h"

bool log_to_file = true;
uint8_t log_level = 0;
struct timespec starup_time;

void reset_logtime() { clock_gettime(CLOCK_MONOTONIC, &starup_time); }

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
  double elapsed_time;
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  elapsed_time = (((current_time.tv_sec - starup_time.tv_sec) * 1e9) +
                  (current_time.tv_nsec - starup_time.tv_nsec)) /
                 1e9; // in seconds

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
      fprintf(fd, "[%.4f] D ", elapsed_time);
      break;
    case 1:
      fprintf(fd, "[%.4f] I ", elapsed_time);
      break;
    case 2:
      fprintf(fd, "[%.4f] W ", elapsed_time);
      break;
    default:
      fprintf(fd, "[%.4f] E ", elapsed_time);
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

void dump_packet(char *direction, uint8_t *buf, int pktsize) {
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
    for (i = 0; i < pktsize; i++) {
      fprintf(fd, "0x%02x ", buf[i]);
    }
    fprintf(fd, "\n");
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

void dump_pkt_raw(uint8_t *buf, int pktsize) {
  int i;
  FILE *fd;
  if (!log_to_file) {
    fd = stdout;
  } else {
    fd = fopen("/var/log/openqti.log", "a");
    if (fd < 0) {
      fprintf(stderr, "[%s] Error opening logfile \n", __func__);
      fd = stdout;
    }
  }
  fprintf(fd, "RAW :");
  for (i = 0; i < pktsize; i++) {
    fprintf(fd, "0x%02x ", buf[i]);
  }
  fprintf(fd, "\n");
  if (fd != stdout) {
    fclose(fd);
  }
}
