// SPDX-License-Identifier: MIT

#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "openqti.h"

#define ADB_EN_MAGIC "persistent_adb_on"
#define PERSIST_USB_AUD_MAGIC "persistent_usbaudio_on"

void logger(uint8_t level, char *format, ...) {}

uint8_t adb_state(void) {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"Error opening misc partition to read adb flag \n");
    return 1;
  }
  
  lseek(fd, 64, SEEK_SET);
  if (read(fd, buff, sizeof(ADB_EN_MAGIC)) < 0) {
    fprintf(stdout, "Can't read ADB state, forcing ADB on\n");
    close(fd);
    return 1;
  }

  close(fd);
  if (strcmp(buff, ADB_EN_MAGIC) == 0) {
    fprintf(stdout, "Persistent ADB is enabled\n");
    return 1;
  }
  
  fprintf(stdout, "Persistent ADB is disabled\n");
  return 0;
}

uint8_t usb_audio_state(void) {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"Error opening misc partition to read audio flag \n");
    return 0;
  }
  lseek(fd, 96, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_USB_AUD_MAGIC)) < 0) {
    fprintf(stdout, "Can't read from the misc partition, disabling USB audio\n");
    close(fd);
    return 0;
  }
  close(fd);
  if (strcmp(buff, PERSIST_USB_AUD_MAGIC) == 0) {
    fprintf(stdout, "USB Audio is enabled\n");
    return 1;
  }
  
  fprintf(stdout, "USB Audio is disabled\n");
  return 0;
}


int main(int argc, char **argv) {
  int ret;
  /* Set initial settings before moving to actual initialization */
  set_initial_config();

  /* Try to read the config file on top of the defaults */
  read_settings_from_file();

  while ((ret = getopt(argc, argv, "aun?")) != -1)
    switch (ret) {
    case 'a':
      return adb_state();

    case 'u':
      return usb_audio_state();

    case 'n':
      ret = is_internal_connect_enabled();
      if (ret) {
        fprintf(stdout, "Internal modem connectivity is enabled\n");
      } else {
        fprintf(stdout, "Internal modem connectivity is disabled\n");
      }
      return ret;

    default:
    case '?':
      fprintf(stdout, "openQTI config reader version %s \n", RELEASE_VER);
      fprintf(stdout, "Options:\n");
      fprintf(stdout, " -a: Is ADB enabled?\n");
      fprintf(stdout, " -u: Is USB audio enabled?\n");
      fprintf(stdout, " -n: Is internal networking enabled?\n");
      fprintf(stdout, " -v: Show build version\n");
      fprintf(stdout, " -?: Show this list\n");
      return 0;
    }



  return 0;
}