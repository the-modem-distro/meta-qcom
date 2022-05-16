// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "../inc/config.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/call.h"

bool log_to_file = true;
uint8_t log_level = 0;
struct timespec startup_time;

void reset_logtime() { clock_gettime(CLOCK_MONOTONIC, &startup_time); }

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

uint8_t get_log_level() {
  return log_level;
}

double get_elapsed_time() {
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  return (((current_time.tv_sec - startup_time.tv_sec) * 1e9) +
          (current_time.tv_nsec - startup_time.tv_nsec)) /
         1e9; // in seconds
}
void logger(uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  double elapsed_time;
  struct timespec current_time;
  int persist = use_persistent_logging();
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  elapsed_time = (((current_time.tv_sec - startup_time.tv_sec) * 1e9) +
                  (current_time.tv_nsec - startup_time.tv_nsec)) /
                 1e9; // in seconds

  if (level >= log_level) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      if (persist) {
        fd = fopen(PERSISTENT_LOGPATH, "a");
      } else {
        fd = fopen(VOLATILE_LOGPATH, "a");
      }
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
    fprintf(fd, "RAW :");
    for (i = 0; i < pktsize; i++) {
      fprintf(fd, "0x%02x ", buf[i]);
    }
    fprintf(fd, "\n");
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

int mask_phone_number(char *orig, char *dest) {
  int orig_size = strlen(orig);
  if (orig_size < 1) {
    snprintf(dest, MAX_PHONE_NUMBER_LENGTH, "[none]");
    return -1;
  }
  snprintf(dest, MAX_PHONE_NUMBER_LENGTH, "%s", orig);
  if (get_log_level() != MSG_DEBUG) {
    for (int i = 0; i < (orig_size - 3); i++) {
      dest[i] = '*';
    }
  }

  logger(MSG_DEBUG, "%s: %s --> %s (%i)\n", __func__, orig, dest, orig_size);
  return 0;
}