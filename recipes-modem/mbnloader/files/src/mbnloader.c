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

int main(int argc, char **argv) {
  FILE *fp;
  int filesize, smdfd, buf_bytes, written_bytes, ret;
  char command[64];
  char buff[2048];
  char response[128];

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
  ret = read(smdfd, &response, 128);
  fprintf(stdout, "We dont care if it fails, we just keep going (res was %s)\n", response);
  memset(response, 0, sizeof(response));
  snprintf(command, sizeof(command), "%s\"RAM:volte_profile.mbn\",%i\r\n",
           UPLOAD_CMD, filesize);
  fprintf(stdout, "Sending command %s to the modem\n", command);
  written_bytes = write(smdfd, command, sizeof(command));
  ret = read(smdfd, &response, 128);
  if (strstr(response, "ERROR") != NULL) {
    fprintf(stderr, "Modem returned an error!: %s\n", response);
    return 0;
  } else if (strstr(response, "CME ERROR") != NULL) {
    fprintf(stderr, "Modem returned a CME Err!: %s\n", response);
    return 0;
  } else if (strstr(response, "CONNECT") != NULL) {
    fprintf(stderr, "Modem is ready for transfer %s\n", response);
  } else {
    fprintf(stderr, "RE:%i: %s\n", ret, response);
  }
  memset(response, 0, sizeof(response));

  do {
    buf_bytes = fread(buff, 1, 2048, fp);
    // fprintf(stdout, "Sending file... %i bytes \n [[ %s \n ]]\n", buf_bytes,
    // buff);
    if (buf_bytes > 0) {
      // Dump to serial
      written_bytes = write(smdfd, buff, buf_bytes);
      if (written_bytes != buf_bytes) {
        fprintf(stderr, "Whoops, written bytes (%i) differ of read bytes (%i)",
                written_bytes, buf_bytes);
      }

    //  ret = read(smdfd, &response, 128);
    //  fprintf(stderr, "RE:%i: %s\n", ret, response);
    //  memset(response, 0, sizeof(response));

    //  if (strstr(response, "ERROR") != NULL) {
    //    fprintf(stderr, "Modem returned an error!: %s\n", response);
    //    return 0;
    //  }
    }
  } while (buf_bytes > 0);

  written_bytes = write(smdfd, LOAD_AS_MBN_CMD, sizeof(LOAD_AS_MBN_CMD));

  fprintf(stdout, "Exiting\n");
  close(smdfd);
  fclose(fp);
  return 0;
}
