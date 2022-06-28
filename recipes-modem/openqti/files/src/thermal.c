// SPDX-License-Identifier: MIT
#include "../inc/thermal.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/config.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include "../inc/scheduler.h"
#include "../inc/sms.h"
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

int get_temperature(char *sensor_path) {
  int fd, val = 0;
  char readval[6];
  fd = open(sensor_path, O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open sysfs entry %s\n", __func__,
           sensor_path);
    return -EINVAL;
  }
  lseek(fd, 0, SEEK_SET);
  read(fd, &readval, 6);
  val = strtol(readval, NULL, 10);

  close(fd);
  return val;
}

void *thermal_monitoring_thread() {
  int sensors[7] = {0};
  int prev_sensor_reading[7] = {0};
  char sensor_path[128];
  int i;
  uint8_t *reply = calloc(160, sizeof(unsigned char));
  int msglen;
  bool notified_emergency = false;
  bool notified_warning = false;
  uint32_t round = 0;
  uint32_t last_notify_round = 0;
  logger(MSG_INFO, "Thermal monitor thread started\n");
  while (1) {
    for (i = 0; i < NO_OF_SENSORS; i++) {
      snprintf(sensor_path, sizeof(sensor_path), "%s%i%s", THRM_ZONE_PATH, i,
               THRM_ZONE_TRAIL);
      prev_sensor_reading[i] = sensors[i];
      sensors[i] = get_temperature(sensor_path);
    }
    if (round % 2 == 0) {
      log_thermal_status(MSG_INFO, "Zones 0-6: %iC %iC %iC %iC %iC %iC %iC \n",
             sensors[0], sensors[1], sensors[2], sensors[3], sensors[4],
             sensors[5], sensors[6]);
    }
    for (i = 0; i < NO_OF_SENSORS; i++) {
      if (prev_sensor_reading[i] != 0 && sensors[i] != 0) {
        if (sensors[i] > THERMAL_TEMP_CRITICAL) {
          logger(
              MSG_ERROR,
              "%s: ERROR: Temperature threshold exceeded. Will shutdown now\n",
              __func__);
          msglen =
              snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "I'm at %iC, I will shutdown now to help the phone cool "
                       "down. Restart eg25-manager to power me up again\n",
                       sensors[i]);
          add_message_to_queue(reply, msglen);
          last_notify_round = round;
          notified_emergency = true;
          // We stop here for a little while
          // If everything is overheated ModemManager will need more time to
          // retrieve the message
          sleep(30);
          syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                  LINUX_REBOOT_CMD_POWER_OFF, NULL);
        }
        if (sensors[i] > THERMAL_TEMP_WARNING && !notified_emergency) {
          msglen = snprintf(
              (char *)reply, MAX_MESSAGE_SIZE,
              "I'm starting to overheat!\nIf temperature keeps raising I will "
              "shut down soon (Sensor %i reports %iC)\n",
              i, sensors[i]);
          add_message_to_queue(reply, msglen);
          last_notify_round = round;
          notified_emergency = true;
        } else if (sensors[i] > THERMAL_TEMP_INFO && !notified_warning) {
          msglen =
              snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "It's getting hot in here... (Sensor %i reports %iC)\n",
                       i, sensors[i]);
          add_message_to_queue(reply, msglen);
          last_notify_round = round;
          notified_warning = true;
        } else if (sensors[i] - prev_sensor_reading[i] > 10) {
          msglen = snprintf(
              (char *)reply, MAX_MESSAGE_SIZE,
              "Temperature is rapidly increasing\nSensor %i:\n  Current "
              "temperature: %iC\n  Previous temperature: %iC\n",
              i, sensors[i], prev_sensor_reading[i]);
          add_message_to_queue(reply, msglen);
          last_notify_round = round;
        }
      }
    }

    sleep(5);
    round++;
    if (round - last_notify_round > 60) {
      notified_emergency = false;
      notified_warning = false;
      last_notify_round = 0;
      round = 0;
    }
  }

  return 0;
}
