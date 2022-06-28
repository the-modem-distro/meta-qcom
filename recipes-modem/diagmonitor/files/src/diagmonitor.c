// SPDX-License-Identifier: MIT

#include "diagmonitor.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*
 *  Diag Monitor
 *  Based on GSM-Parser https://github.com/E3V3A/gsm-parser
 *  and
 *  Snoop Snitch https://github.com/E3V3A/snoopsnitch
 *
 */

/* Remove this when merging to openqti */
#define MAX_FD 9999
#define DIAG_IOCTL_SWITCH_LOGGING 7
#define MEMORY_DEVICE_MODE 2
struct diag_logging_mode_param_t stMode = {MEMORY_DEVICE_MODE, 0, 1};
struct {
  bool run;
} diag_runtime;

void start_diag_thread() {
  int diagfd = -1;
  int pret, ret;
  fd_set readfds;
  struct timeval tv;
  char buffer[MAX_BUF_SIZE];

  fprintf(stdout, "%s: Starting...\n", __func__);
  diagfd = open(DIAG_INTERFACE, O_RDWR);
  if (!diagfd) {
    fprintf(stderr, "%s: Error opening diag interface!\n", __func__);
    return;
  }
  /* Initialize */

  ret = ioctl(diagfd, DIAG_IOCTL_SWITCH_LOGGING, MEMORY_DEVICE_MODE);
  if (ret < 0) {
    fprintf(stderr, "%s: Error setting diag to memory (%i)\n", __func__, ret);
    ret = ioctl(diagfd, DIAG_IOCTL_SWITCH_LOGGING, (void *)&stMode);
  }
  if (ret < 0) {
    fprintf(stderr, "%s: Error setting diag to memory again (%i)\n", __func__,
            ret);
  }

  /* Now we loop */
  while (diag_runtime.run) {
    FD_ZERO(&readfds);
    memset(buffer, 0, MAX_BUF_SIZE);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    FD_SET(diagfd, &readfds);
    pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(diagfd, &readfds)) {
      fprintf(stdout, "%s: Data at diag interface\n", __func__);
      ret = read(diagfd, &buffer, MAX_BUF_SIZE);
      if (ret > 0) {
        for (int i = 0; i < ret; i++) {
          fprintf(stdout, "%.2x ", buffer[i]);
        }
        fprintf(stdout, "\n End of packet\n");
      }
    }
  }

  close(diagfd);
  fprintf(stdout, "Finish.\n");
}
int main(int argc, char **argv) {
  diag_runtime.run = true;
  start_diag_thread();
  return 0;
}
