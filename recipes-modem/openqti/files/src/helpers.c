// SPDX-License-Identifier: MIT

#include "../inc/helpers.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/tracking.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

int is_usb_suspended = 0;

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
  if (write(fd, (void *)tmpbuff, sizeof(fbcmd)) < 0) {
    logger(MSG_ERROR, "%s: Error writing the reboot flag \n", __func__);
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

int get_transceiver_suspend_state() {
  int fd, val = 0;
  char readval[6];
  is_usb_suspended = 0;
  fd = open("/sys/devices/78d9000.usb/msm_hsusb/isr_suspend_state", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open USB state \n", __func__);
    return 0; // assume active
  }
  lseek(fd, 0, SEEK_SET);
  if (read(fd, &readval, 1) <= 0) {
    logger(MSG_ERROR, "%s: Error reading USB Sysfs entry \n", __func__);
  }
  val = strtol(readval, NULL, 10);

  if (val > 0 && is_usb_suspended == 0) {
    is_usb_suspended = 1; // USB is suspended, stop trying to transfer data
  } else if (val == 0 && is_usb_suspended == 1) {
    usleep(1000);         // Allow time to finish wakeup
    is_usb_suspended = 0; // Then allow transfers again
  }
  close(fd);
  return 0;
}

void *gps_proxy() {
  struct node_pair *nodes;
  nodes = calloc(1, sizeof(struct node_pair));
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[64];
  char node2_to_1[64];

  /* Set the names */
  strncpy(nodes->node1.name, "Modem GPS", sizeof("Modem GPS"));
  strncpy(nodes->node2.name, "USB-GPS", sizeof("USB-GPS"));
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
           nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
           nodes->node2.name);

  while (1) {
    get_transceiver_suspend_state();
    if (!is_usb_suspended) {
      logger(MSG_INFO, "%s: Initialize GPS proxy thread.\n", __func__);
      nodes->allow_exit = false;
      nodes->node1.fd = open(SMD_GPS, O_RDWR);
      if (nodes->node1.fd < 0) {
        logger(MSG_ERROR, "%s: Error opening %s \n", __func__, SMD_GPS);
        nodes->allow_exit = true;
      }

      nodes->node2.fd = open(USB_GPS, O_RDWR);
      if (nodes->node2.fd < 0) {
        logger(MSG_ERROR, "%s: Error opening %s \n", __func__, USB_GPS);
        nodes->allow_exit = true;
      }
      if (nodes->allow_exit) {
        logger(MSG_ERROR, "%s: One of the descriptors isn't ready\n", __func__);
        usleep(10000);
      }
    } else {
      nodes->allow_exit = true;
    }

    while (!nodes->allow_exit) {
      get_transceiver_suspend_state();
      if (!is_usb_suspended) {
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
            logger(MSG_ERROR, "%s: Closing at the ADSP side \n", __func__);
            nodes->allow_exit = true;
            close(nodes->node1.fd);
          }
        } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
          ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
          if (ret > 0) {
            dump_packet(node2_to_1, buf, ret);
            ret = write(nodes->node1.fd, buf, ret);
          } else {
            logger(MSG_ERROR, "%s: Closing at the USB side \n", __func__);
            nodes->allow_exit = true;
            close(nodes->node2.fd);
          }
        }
      }
    }
  }
}

void *rmnet_proxy(void *node_data) {
  struct node_pair *nodes = (struct node_pair *)node_data;
  int pret, ret;
  bool looped = true;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[64];
  char node2_to_1[64];
  logger(MSG_INFO, "%s: Initialize RMNET proxy thread.\n", __func__);
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
           nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
           nodes->node2.name);
  while (1) {
    get_transceiver_suspend_state();
    // Everything dies if I remove this?
    while (!nodes->allow_exit) {
      get_transceiver_suspend_state();
      if (!is_usb_suspended) {
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
            if (ret == 0) {
              logger(MSG_WARN, "%s: Failed to write at the USB side: %i \n",
                     __func__, ret);
            }
          } else {
            logger(MSG_ERROR, "%s: Closed descriptor at the USB side: %i \n",
                   __func__, ret);
          }
        } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
          ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
          if (ret > 0) {
            handle_call_pkt(buf, FROM_DSP, ret);
            track_client_count(buf, FROM_DSP, ret, nodes->node2.fd,
                               nodes->node1.fd);
            dump_packet(node2_to_1, buf, ret);
            ret = write(nodes->node1.fd, buf, ret);
            if (ret == 0) {
              logger(MSG_WARN, "%s: Failed to write at the ADSP side: %i \n",
                     __func__, ret);
            }
          } else {
            logger(MSG_ERROR, "%s: Closed descriptor at the ADSP side: %i \n",
                   __func__, ret);
          }
        }
      }
    }
  } // end of infinite loop

  return NULL;
}
