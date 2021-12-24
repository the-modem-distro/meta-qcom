// SPDX-License-Identifier: MIT

#include "mbnloader.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/*
 *  MBN Loader
 *    Small utility to push IMS profiles to the modem
 *
 *
 */
int is_ok(int smdfd) {
  char response[128];
  int ret;
  fprintf(stdout, "Reading response...\n");
  ret = read(smdfd, &response, 128);
  if (ret == 0) {
    fprintf(stdout, "No response\n");
  }
  if (strstr(response, "OK") != NULL) {
    fprintf(stdout, "OK Response\n");
    return 1;
  } else if (strstr(response, "CONNECT") != NULL) {
    fprintf(stdout, "CONNECT Response\n");
    return 1;
  } else if (strstr(response, "ERROR") != NULL) {
    fprintf(stdout, "ERROR Response\n");
    return 0;
  } else if (strstr(response, "CME ERROR") != NULL) {
    fprintf(stdout, "CME ERROR Response: %s\n", response);
    return 0;
  }
  fprintf(stderr, "No response\n");
  return -1;
}

int main(int argc, char **argv) {
  FILE *fp;
  int filesize, smdfd, buf_bytes, written_bytes, ret;
  char command[64];
  char *buff;

  fprintf(stdout, "MBN Loader\n");
  if (argc < 2) {
    fprintf(stderr, "Please specify a file to load\n");
    return 0;
  }

  fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "Can't open file %s\n", argv[1]);
    return 0;
  }
  fseek(fp, 0L, SEEK_END);
  filesize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  if (filesize == 0) {
    fprintf(stderr, "%s: File is empty!\n", __func__);
    return -EINVAL;
  }
  buff = calloc(filesize, sizeof(char));

  smdfd = open(SMD_DEVICE_PATH, O_RDWR);
  if (smdfd < 0) {
    fprintf(
        stderr,
        "%s: Cannot open device %s, are you running this inside the modem?\n",
        __func__, SMD_DEVICE_PATH);
    return -EINVAL;
  }

  snprintf(command, sizeof(command), "%s\"RAM:volte_profile.mbn\"\r\n",
           DELETE_CMD);
  fprintf(stdout, "Sending command %s to the modem\n", command);
  written_bytes = write(smdfd, command, sizeof(command));
  do {
    fprintf(stdout, "Waiting for an answer...\n");
  } while (is_ok(smdfd) == -1);

  snprintf(command, sizeof(command), "%s\"RAM:volte_profile.mbn\",%i\r\n",
           UPLOAD_CMD, filesize);
  fprintf(stdout, "Sending command %s to the modem\n", command);
  written_bytes = write(smdfd, command, sizeof(command));
   do {
    fprintf(stdout, "Waiting for an answer...\n");
    ret = is_ok(smdfd);
  } while (ret == -1);
  if (!ret) {
    fprintf(stderr, "Modem returned an error\n");
    return 0;
  }

  do {
    buf_bytes = fread(buff, 1, filesize, fp);
    if (buf_bytes > 0) {
      // Dump to serial
      written_bytes = write(smdfd, buff, buf_bytes);
      if (written_bytes != buf_bytes) {
        fprintf(stderr, "Whoops, written bytes (%i) differ of read bytes (%i)",
                written_bytes, buf_bytes);
      }
    }
  } while (buf_bytes > 0);

//  written_bytes = write(smdfd, LOAD_AS_MBN_CMD, sizeof(LOAD_AS_MBN_CMD));

  fprintf(stdout, "Exiting\n");
  close(smdfd);
  fclose(fp);
  return 0;
}
