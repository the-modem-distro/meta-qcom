// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#define ADB_EN_MAGIC "persistent_adb_on"

int main(int argc, char **argv) {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"Error opening misc partition to read adb flag \n");
    return -EINVAL;
  }
  lseek(fd, 64, SEEK_SET);
  read(fd, buff, sizeof(ADB_EN_MAGIC));
  close(fd);
  if (strcmp(buff, ADB_EN_MAGIC) == 0) {
    fprintf(stdout, "Persistent ADB is enabled\n");
    return 1;
  }
  
  fprintf(stdout, "Persistent ADB is disabled\n");
  return 0;
}

