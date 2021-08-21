// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#define PERSIST_USB_AUD_MAGIC "persistent_usbaudio_on"

int main(int argc, char **argv) {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"Error opening misc partition to read audio flag \n");
    return 0;
  }
  lseek(fd, 96, SEEK_SET);
  read(fd, buff, sizeof(PERSIST_USB_AUD_MAGIC));
  close(fd);
  if (strcmp(buff, PERSIST_USB_AUD_MAGIC) == 0) {
    fprintf(stdout, "USB Audio is enabled\n");
    return 1;
  }
  
  fprintf(stdout, "USB Audio is disabled\n");
  return 0;
}

