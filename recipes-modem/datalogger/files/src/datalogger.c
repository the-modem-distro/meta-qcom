// SPDX-License-Identifier: MIT

#include "datalogger.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int get_temperature(char *sensor_path) {
  int fd, val = 0;
  char readval[6];
  fd = open(sensor_path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "%s: Cannot open sysfs entry %s\n", __func__, sensor_path);
    return -EINVAL;
  }
  lseek(fd, 0, SEEK_SET);
  read(fd, &readval, 6);
  val = strtol(readval, NULL, 10);

  close(fd);
  return val;
}

int main(int argc, char **argv) {
  int sensors[7];
  char sensor_path[128];
  int i;
  FILE *fd;
  fd = fopen(LOG_FILE, "a");
  if (fd < 0) {
    fprintf(stderr, "Error opening logfile \n");
    return 1;
  }

  fprintf(fd, "Data logger: Sensor info dump\n");
  while (1) {
    for (i = 0; i < NO_OF_SENSORS; i++) {
      snprintf(sensor_path, sizeof(sensor_path), "%s%i%s", THRM_ZONE_PATH, i,
               THRM_ZONE_TRAIL);
      sensors[i] = get_temperature(sensor_path);
    }
    for (i = 0; i < NO_OF_SENSORS; i++) {
      fprintf(fd, "Z%i: %iC ", i, sensors[i]);
    }
    fprintf(fd, "\n");
    sleep(5);
    fflush(fd);
  }

  fclose(fd);
  return 0;
}
