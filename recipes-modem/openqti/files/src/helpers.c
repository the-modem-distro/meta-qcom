// SPDX-License-Identifier: MIT

#include "../inc/helpers.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/input.h>
#include <syscall.h>
#include <linux/reboot.h>

int write_to(const char *path, const char *val, int flags) {
  int ret;
  int fd = open(path, flags);
  if (fd < 0) {
    return -ENOENT;
  }
  ret = write(fd, val, strlen(val) * sizeof(char));
  close(fd);
  return ret;
}

uint32_t get_curr_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  uint32_t milliseconds =
      te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
  return milliseconds;
}

int is_adb_enabled() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 1;
  }
  lseek(fd, 64, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_ADB_ON_MAGIC)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading ADB state \n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_ADB_ON_MAGIC) == 0) {
    logger(MSG_DEBUG, "%s: Persistent ADB is enabled\n", __func__);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Persistent ADB is disabled \n", __func__);
  return 0;
}

void store_adb_setting(bool en) {
  char buff[32];
  memset(buff, 0, 32);

  int fd;
  if (en) { // Store the magic string in the second block of the misc partition
    logger(MSG_WARN, "Enabling persistent ADB\n");
    strncpy(buff, PERSIST_ADB_ON_MAGIC, sizeof(PERSIST_ADB_ON_MAGIC));
  } else {
    logger(MSG_WARN, "Disabling persistent ADB\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition to set adb flag \n",
           __func__);
    return;
  }
  lseek(fd, 64, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing the ADB flag \n", __func__);
  }
  close(fd);
}

int get_audio_mode() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return AUDIO_MODE_I2S;
  }
  lseek(fd, 96, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_USB_AUD_MAGIC)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading USB audio state \n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_USB_AUD_MAGIC) == 0) {
    logger(MSG_INFO, "%s: Persistent USB audio is enabled\n", __func__);
    return AUDIO_MODE_USB;
  }

  logger(MSG_INFO, "%s: Persistent USB audio is disabled \n", __func__);
  return AUDIO_MODE_I2S;
}

void store_audio_output_mode(uint8_t mode) {
  char buff[32];
  memset(buff, 0, 32);
  int fd;
  if (mode == AUDIO_MODE_USB) { // Store the magic string in the second block of
                                // the misc partition
    logger(MSG_WARN, "Enabling USB Audio\n");
    strncpy(buff, PERSIST_USB_AUD_MAGIC, sizeof(PERSIST_USB_AUD_MAGIC));
  } else {
    logger(MSG_WARN, "Disabling USB audio\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR,
           "%s: Error opening misc partition to set audio output flag \n",
           __func__);
    return;
  }
  lseek(fd, 96, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing USB audio flag \n", __func__);
  }
  close(fd);
}

int use_custom_alert_tone() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 0;
  }
  lseek(fd, 128, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_CUSTOM_ALERT_TONE)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading alert tone flag\n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_CUSTOM_ALERT_TONE) == 0) {
    logger(MSG_DEBUG, "%s: Using custom alert tone\n", __func__);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Using default alert tone provided by the carrier \n",
         __func__);
  return 0;
}

void set_custom_alert_tone(bool en) {
  char buff[32];
  memset(buff, 0, 32);

  int fd;
  if (en) { // Store the magic string in the second block of the misc partition
    logger(MSG_WARN, "Enabling Custom alert tone\n");
    strncpy(buff, PERSIST_CUSTOM_ALERT_TONE, sizeof(PERSIST_CUSTOM_ALERT_TONE));
  } else {
    logger(MSG_WARN, "Disabling custom alert tone\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR,
           "%s: Error opening misc partition to set alert tone flag \n",
           __func__);
    return;
  }
  lseek(fd, 128, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing the alert tone flag \n", __func__);
  }
  close(fd);
}

int get_modem_name(char *buff) {
  int fd, i;
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 0;
  }
  lseek(fd, 160, SEEK_SET);
  if (read(fd, buff, 32) <= 0) {
    logger(MSG_ERROR, "%s: Error reading modem name\n", __func__);
    strncpy(buff, "Modem", strlen("Modem"));
  }
  close(fd);
  if (buff[0] == '\0') {
    for (i = 1; i < 32; i++) {
      buff[i - 1] = buff[i];
    }
    logger(MSG_INFO, "%s: Your modem name is %s\n", __func__, buff);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Using default name \n", __func__);
  strncpy(buff, "Modem", strlen("Modem"));

  return 1;
}

int get_user_name(char *buff) {
  int fd, i;
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 0;
  }
  lseek(fd, 200, SEEK_SET);
  if (read(fd, buff, 32) <= 0) {
    logger(MSG_ERROR, "%s: Error reading modem name\n", __func__);
    strncpy(buff, "Admin", strlen("Admin"));
  }
  close(fd);
  if (buff[0] == '\0') {
    for (i = 1; i < 32; i++) {
      buff[i - 1] = buff[i];
    }
    logger(MSG_INFO, "%s: Your name is %s\n", __func__, buff);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Using default name \n", __func__);
  strncpy(buff, "Admin", strlen("Admin"));

  return 1;
}

void set_modem_name(char *name) {
  char buff[33];
  memset(buff, 0, 33);

  int fd;
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition \n", __func__);
    return;
  }
  lseek(fd, 160, SEEK_SET);
  snprintf(buff + 1, sizeof(buff) - 1, "%s", name);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing modem's name \n", __func__);
  }
  close(fd);
}

void set_user_name(char *name) {
  char buff[33];
  memset(buff, 0, 33);

  int fd;
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition \n", __func__);
    return;
  }
  lseek(fd, 200, SEEK_SET);
  snprintf(buff + 1, sizeof(buff) - 1, "%s", name);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing user's name \n", __func__);
  }
  close(fd);
}

void reset_usb_port() {
  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }
  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

void restart_usb_stack() {
  int ret;
  char functions[64] = "diag,serial,rmnet";
  if (is_adb_enabled()) {
    strcat(functions, ",ffs");
  }

  if (get_audio_mode()) {
    strcat(functions, ",audio");
  }

  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }

  if (write_to(USB_FUNC_PATH, functions, O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error setting USB functions \n", __func__);
  }

  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }

  // Switch between I2S and usb audio depending on the misc partition setting
  set_output_device(get_audio_mode());
  // Enable or disable ADB depending on the misc partition setting
  set_adb_runtime(is_adb_enabled());

  // ADB should start when usb is available
  if (is_adb_enabled()) {
    if (system("/etc/init.d/adbd start") < 0) {
      logger(MSG_WARN, "%s: Failed to start ADB \n", __func__);
    }
  }
}

void enable_usb_port() {
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

void set_suspend_inhibit(bool mode) {
  if (mode == true) {
    logger(MSG_INFO, "%s: Blocking USB suspend\n", __func__);
    if (write_to(SUSPEND_INHIBIT_PATH, "1", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting USB suspend \n", __func__);
    }
  } else {
    logger(MSG_INFO, "%s: USB suspend is allowed\n", __func__);
    if (write_to(SUSPEND_INHIBIT_PATH, "0", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting USB suspend \n", __func__);
    }
  }
}

char *get_gpio_dirpath(char *gpio) {
  char *path;
  path = calloc(256, sizeof(char));
  snprintf(path, 256, "%s%s/%s", GPIO_SYSFS_BASE, gpio, GPIO_SYSFS_DIRECTION);

  return path;
}
void prepare_dtr_gpio() {
  logger(MSG_INFO, "%s: Getting GPIO ready\n", __func__);
  if (write_to(GPIO_EXPORT_PATH, GPIO_DTR, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error exporting GPIO_DTR pin\n", __func__);
  }
}

char *get_gpio_value_path(char *gpio) {
  char *path;
  path = calloc(256, sizeof(char));
  snprintf(path, 256, "%s%s/%s", GPIO_SYSFS_BASE, gpio, GPIO_SYSFS_VALUE);
  return path;
}

uint8_t get_dtr_state() {
  int dtr, ret;
  char dtrval;
  dtr = open(get_gpio_value_path(GPIO_DTR), O_RDONLY | O_NONBLOCK);
  if (dtr < 0) {
    logger(MSG_WARN, "%s: DTR not available: %s \n", __func__,
           get_gpio_value_path(GPIO_DTR));
    return 0;
  }

  lseek(dtr, 0, SEEK_SET);
  if (read(dtr, &dtrval, 1) < 0) {
        logger(MSG_WARN, "%s: Error reading DTR value \n", __func__);
  }
  if ((int)(dtrval - '0') == 1) {
    ret = 1;
  } else {
    ret = 0;
  }

  close(dtr);
  return ret;
}

uint8_t pulse_ring_in() {
  int i;
  logger(MSG_INFO, "%s: [[[RING IN]]]\n", __func__);

  if (write_to(GPIO_EXPORT_PATH, GPIO_RING_IN, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error exporting GPIO_RING_IN pin\n", __func__);
  }
  if (write_to(get_gpio_dirpath(GPIO_RING_IN), GPIO_MODE_OUTPUT, O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error set dir: GPIO_WAKEUP_IN pin\n", __func__);
  }
  usleep(300);
  for (i = 0; i < 10; i++) {
    if (i % 2 == 0) {
      if (write_to(get_gpio_value_path(GPIO_RING_IN), "1", O_RDWR) < 0) {
        logger(MSG_ERROR, "%s: Error Writing to Ring in\n", __func__);
      }
    } else {
      if (write_to(get_gpio_value_path(GPIO_RING_IN), "0", O_RDWR) < 0) {
        logger(MSG_ERROR, "%s: Error Writing to Ring in\n", __func__);
      }
    }
    usleep(300);
  }

  usleep(300);
  /* if (write_to(get_gpio_dirpath(GPIO_RING_IN), GPIO_MODE_INPUT, O_WRONLY) <
   0) { logger(MSG_ERROR, "%s: Error set dir: GPIO_WAKEUP_IN pin at %s\n",
            __func__);
   }
 */
  if (write_to(GPIO_UNEXPORT_PATH, GPIO_RING_IN, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error unexporting GPIO_RING_IN pin\n", __func__);
  }
  return 0;
}

int elapsed_time(struct timeval *prev, struct timeval *cur) {
  if (cur->tv_usec > prev->tv_usec) {
    cur->tv_usec += 1000000;
    cur->tv_sec--;
  }
  return (int)(cur->tv_sec - prev->tv_sec) * 1000000 + cur->tv_usec -
         prev->tv_usec;
}

void *power_key_event() {
  int fd = 0;
  struct input_event ev;
  struct timeval prev;
  struct timeval cur;
  int ret = 0;
  int duration = 0;
  char *arg1 = NULL;
  memset(&prev, 0, sizeof(struct timeval));
  memset(&cur, 0, sizeof(struct timeval));

  fd = open(INPUT_DEV, O_RDONLY);
  if (fd == -1) {
    logger(MSG_ERROR, "%s: Error opening %s\n", __func__, INPUT_DEV);
    return NULL;
  }

  while ((ret = read(fd, &ev, sizeof(struct input_event))) > 0) {
    if (ret < sizeof(struct input_event)) {
      logger(MSG_ERROR, "%s: cannot read whole input event\n", __func__);
    } else {

    if (ev.type == EV_KEY && ev.code == KEY_POWER && ev.value == 1) {
      memcpy(&prev, &ev.time, sizeof(struct timeval));
    } else if (ev.type == EV_KEY && ev.code == KEY_POWER && ev.value == 0) {
      memcpy(&cur, &ev.time, sizeof(struct timeval));
      if (elapsed_time(&prev, &cur) > 1000000) {
        logger(MSG_ERROR, "%s: Suspend/Resume\n", __func__);
        system("echo mem > /sys/power/state");
      } else {
        logger(MSG_ERROR, "%s: Poweroff requested!\n", __func__);
        syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);

      }
    }
  }
  }

 return NULL;
}