// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/tracking.h"

char prev_dtr, prev_wakeup, prev_sleepind;

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
  read(fd, buff, sizeof(PERSIST_ADB_ON_MAGIC));
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
  int fd;
  if (en) { // Store the magic string in the second block of the misc partition
    logger(MSG_WARN, "Enabling persistent ADB\n");
    strncpy(buff, PERSIST_ADB_ON_MAGIC, sizeof(PERSIST_ADB_ON_MAGIC));
  } else {
    logger(MSG_WARN, "Disabling persistent ADB\n");
    strncpy(buff, PERSIST_ADB_OFF_MAGIC, sizeof(PERSIST_ADB_OFF_MAGIC));
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition to set adb flag \n",
           __func__);
    return;
  }
  lseek(fd, 64, SEEK_SET);
  write(fd, &buff, sizeof(buff));
  close(fd);
}

void set_next_fastboot_mode(int flag) {
  struct fastboot_command fbcmd;
  void *tmpbuff;
  tmpbuff = calloc(64, sizeof(char));
  int fd;
  if (flag == 0) { // Reboot to fastboot mode
    strncpy(fbcmd.command, "boot_fastboot", sizeof(fbcmd.command));
    strncpy(fbcmd.status, "force", sizeof(fbcmd.status));
  } else if (flag == 1) { // reboot to recovery
    strncpy(fbcmd.command, "boot_recovery", sizeof(fbcmd.command));
    strncpy(fbcmd.status, "force", sizeof(fbcmd.status));
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR,
           "%s: Error opening misc partition to set reboot flag %i \n",
           __func__, flag);
    return;
  }
  lseek(fd, 131072, SEEK_SET);
  tmpbuff = &fbcmd;
  write(fd, (void *)tmpbuff, sizeof(fbcmd));
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
  read(fd, buff, sizeof(PERSIST_USB_AUD_MAGIC));
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
  write(fd, &buff, sizeof(buff));
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
}

void enable_usb_port() {
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

char *get_gpio_value_path(char *gpio) {
  char *path;
  path = calloc(256, sizeof(char));
  snprintf(path, 256, "%s%s/%s", GPIO_SYSFS_BASE, gpio, GPIO_SYSFS_VALUE);

  return path;
}

int get_dtr() {
  int dtr, val = 0;
  char readval;
  dtr = open(get_gpio_value_path(GPIO_DTR), O_RDONLY);
  if (dtr < 0) {
    logger(MSG_ERROR, "%s: DTR not available: %s \n", __func__,
           get_gpio_value_path(GPIO_DTR));
    return 0; // assume active
  }
  lseek(dtr, 0, SEEK_SET);
  read(dtr, &readval, 1);
  close(dtr);
  if ((int)(readval - '0') == 1) {
    val = 1;
  }
  return val;
}
int get_wakeup_ind() {
  int dtr, val = 0;
  char readval;
  dtr = open(get_gpio_value_path(GPIO_WAKEUP_IN), O_RDONLY);
  if (dtr < 0) {
    logger(MSG_ERROR, "%s: GPIO_WAKEUP_IN not available: %s \n", __func__,
           get_gpio_value_path(GPIO_WAKEUP_IN));
    return 0; // assume active
  }
  lseek(dtr, 0, SEEK_SET);
  read(dtr, &readval, 1);
  close(dtr);
  if ((int)(readval - '0') == 1) {
    val = 1;
    logger(MSG_INFO, "%s: WAKEUP is 1 \n", __func__);
  }
  logger(MSG_INFO, "%s: WAKEUP is %i \n", __func__, val);

  return val;
}
void *handle_gpios() {
  int sleep_ind, wakeup_in, dtr, bitset, fd;
  char dtrval, wakeupval;
  if (prev_dtr != '0' && prev_dtr != '1') {
    prev_dtr = '0';
    prev_wakeup = '0';
    prev_sleepind = '0';
  }
  dtr = open(get_gpio_value_path(GPIO_DTR), O_RDONLY | O_NONBLOCK);
  wakeup_in = open(get_gpio_value_path(GPIO_WAKEUP_IN), O_RDONLY | O_NONBLOCK);
  if (dtr < 0) {
    logger(MSG_ERROR, "%s: DTR not available: %s \n", __func__,
           get_gpio_value_path(GPIO_DTR));
  } else {
    lseek(dtr, 0, SEEK_SET);
    read(dtr, &dtrval, 1);
    close(dtr);
  }
  if (wakeup_in < 0) {
    logger(MSG_ERROR, "%s: WAKEUP in not available %s: \n", __func__,
           get_gpio_value_path(GPIO_WAKEUP_IN));
  } else {
    lseek(wakeup_in, 0, SEEK_SET);
    read(wakeup_in, &wakeupval, 1);
    close(wakeup_in);
  }

  if (dtrval != prev_dtr || wakeupval != prev_wakeup) {
    logger(MSG_INFO, "%s: DTR: %c->%c WAKEUP_IND: %c->%c\n", __func__, prev_dtr,
           dtrval, prev_wakeup, wakeupval);
    prev_dtr = dtrval;
    prev_wakeup = wakeupval;
     fd = open(SMD_DATA3, O_RDWR);
    if (fd < 0) {
      logger(MSG_ERROR, "%s: DATA3 is not available \n", __func__);
    } else {
      logger(MSG_ERROR, "%s: DATA3 is available, continuing \n", __func__);
      if (dtrval == 0) {
        bitset = 2;
      } else {
        bitset = 0;
      }
      if (ioctl(fd, TIOCMSET, &bitset) == -1) {
            logger(MSG_ERROR, "%s: IOCTL Failed for set bit %i \n", __func__,
                   bitset);
      }

      close(fd);
    }
  }

  return NULL;
}

void *gps_proxy() {
  struct node_pair *nodes;
  nodes = calloc(1, sizeof(struct node_pair));
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[32];
  char node2_to_1[32];
  while (1) {
    logger(MSG_INFO, "%s: Initialize GPS proxy thread.\n", __func__);

    /* Set the names */
    strncpy(nodes->node1.name, "Modem GPS", sizeof("Modem GPS"));
    strncpy(nodes->node2.name, "USB-GPS", sizeof("USB-GPS"));
    snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
             nodes->node2.name);
    snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
             nodes->node2.name);

    nodes->node1.fd = open(SMD_GPS, O_RDWR);
    if (nodes->node1.fd < 0) {
      logger(MSG_ERROR, "Error opening %s \n", SMD_GPS);
    }

    nodes->node2.fd = open(USB_GPS, O_RDWR);
    if (nodes->node2.fd < 0) {
      logger(MSG_ERROR, "Error opening %s \n", USB_GPS);
    }

    if (nodes->node1.fd >= 0 && nodes->node2.fd >= 0) {
      nodes->allow_exit = false;
    } else {
      logger(MSG_ERROR, "One of the descriptors isn't ready\n");
      nodes->allow_exit = true;
      sleep(1);
    }

    while (!nodes->allow_exit) {
      handle_gpios();
      //  if (!get_dtr()) {
      FD_ZERO(&readfds);
      memset(buf, 0, sizeof(buf));
      FD_SET(nodes->node1.fd, &readfds);
      FD_SET(nodes->node2.fd, &readfds);
      pret = select(MAX_FD, &readfds, NULL, NULL, NULL);
      if (FD_ISSET(nodes->node1.fd, &readfds)) {
        ret = read(nodes->node1.fd, &buf, MAX_PACKET_SIZE);
        if (ret > 0) {
          dump_packet(node1_to_2, buf, ret);
          ret = write(nodes->node2.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closing descriptor at the ADSP side \n",
                 __func__);
          nodes->allow_exit = true;
        }
      } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
        ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
        if (ret > 0) {
          dump_packet(node2_to_1, buf, ret);
          ret = write(nodes->node1.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closing descriptor at the USB side \n",
                 __func__);
          nodes->allow_exit = true;
        }
      }
      //    }
    }
    logger(MSG_ERROR,
           "%s: One of the descriptors was closed, restarting the thread \n",
           __func__);
    close(nodes->node1.fd);
    close(nodes->node2.fd);
  }
}

void *rmnet_proxy(void *node_data) {
  struct node_pair *nodes = (struct node_pair *)node_data;
  int pret, ret;
  bool looped = true;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[32];
  char node2_to_1[32];
  logger(MSG_INFO, "%s: Initialize RMNET proxy thread.\n", __func__);
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
           nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
           nodes->node2.name);
  while (1) {
    while (!nodes->allow_exit) {
      handle_gpios();
      FD_ZERO(&readfds);
      memset(buf, 0, sizeof(buf));
      FD_SET(nodes->node1.fd, &readfds);
      FD_SET(nodes->node2.fd, &readfds);
      pret = select(MAX_FD, &readfds, NULL, NULL, NULL);
      if (FD_ISSET(nodes->node1.fd, &readfds)) {
        ret = read(nodes->node1.fd, &buf, MAX_PACKET_SIZE);
        if (ret > 0) {
          track_client_count(buf, FROM_HOST, ret, nodes->node2.fd,
                             nodes->node1.fd);
          dump_packet(node1_to_2, buf, ret);
          ret = write(nodes->node2.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closed descriptor at the USB side: %i \n",
                 __func__, ret);
          nodes->allow_exit = true;
        }
      } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
        ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
        if (ret > 0) {
          handle_call_pkt(buf, FROM_DSP, ret);
          track_client_count(buf, FROM_DSP, ret, nodes->node2.fd,
                             nodes->node1.fd);
          dump_packet(node2_to_1, buf, ret);
          ret = write(nodes->node1.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closed descriptor at the ADSP side: %i \n",
                 __func__, ret);
          nodes->allow_exit = true;
        }
      }
    }
    logger(MSG_WARN, "RMNET proxy is dead! Trying to recover\n");
    close(nodes->node1.fd);
    close(nodes->node2.fd);
    nodes->allow_exit = false;

    nodes->node2.fd = open(SMD_CNTL, O_RDWR);
    if (nodes->node2.fd < 0) {
      logger(MSG_ERROR, "Error recoverying %s \n", SMD_CNTL);
      nodes->allow_exit = true;
    }
    nodes->node1.fd = open(RMNET_CTL, O_RDWR);
    if (nodes->node1.fd < 0) {
      logger(MSG_ERROR, "Error recoverying %s \n", RMNET_CTL);
      nodes->allow_exit = true;
    }
    if (!nodes->allow_exit) {
      force_close_qmi(nodes->node2.fd); // kill everything when this happens
    } else {
      logger(MSG_ERROR,
             "Cannot force close QMI clients: ADSP not available.\n");
    }
  } // end of infinite loop

  return NULL;
}

void *dtr_monitor() {
  int count = 0;
  logger(MSG_INFO, "%s: Initialize DTR status monitor thread.\n", __func__);
  int fd, dtr, ret;
  struct pollfd fdset[1];
  char dtrval;
  int bitset;
  dtr = open(get_gpio_value_path(GPIO_DTR), O_RDONLY | O_NONBLOCK);
  if (dtr < 0) {
    logger(MSG_ERROR, "%s: DTR not available: %s \n", __func__,
           get_gpio_value_path(GPIO_DTR));
    return NULL;
  }
  while (1) {
    logger(MSG_ERROR, "%s: Looped thread: %i \n", __func__, count);
    count++;
    fdset[0].fd = dtr;
    fdset[0].events = POLLPRI;
    logger(MSG_INFO, "%s: Poll wait\n", __func__);
    ret = poll(fdset, 1, -1);
    fd = open(SMD_DATA3, O_RDWR);
    if (fd < 0) {
      logger(MSG_ERROR, "%s: DATA3 is not available \n", __func__);
    } else {
      logger(MSG_ERROR, "%s: DATA3 is available, continuing \n", __func__);
    }
    if (fdset[0].revents & POLLPRI) {
      logger(MSG_INFO, "%s: Poll pri\n", __func__);

      lseek(dtr, 0, SEEK_SET);
      read(dtr, &dtrval, 1);
      if ((int)dtrval - '0' == 0) { // 0?
        bitset = 2;
        logger(MSG_ERROR, "%s: DTR  is 0 \n", __func__);
        if (fd >= 0) {
          if (ioctl(fd, TIOCMSET, &bitset) == -1) {
            logger(MSG_ERROR, "%s: IOCTL Failed for set bit %i \n", __func__,
                   bitset);
          }
        }
        } else if ((int)dtrval - '0' == 1) { // 2?
          bitset = 0;
          logger(MSG_ERROR, "%s: DTR  is 1 \n", __func__);
            if (fd >= 0) {
              if (ioctl(fd, TIOCMSET, &bitset) == -1) {
                logger(MSG_ERROR, "%s: IOCTL Failed for set bit %i \n",
                       __func__, bitset);
              }
            }
          } else {
            logger(MSG_ERROR, "%s: ERR: DTR  is %c \n", __func__, dtrval);

          } //   if ((int)(readval - '0') == 1) {
        }
        logger(MSG_INFO, "%s: End of loop\n", __func__);
        if (fd >= 0) {
          close(fd);
        }
        handle_gpios();
      } // end of infinite loop

      close(dtr);
      return NULL;
    }