// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"

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
    return -EINVAL;
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

void *gps_proxy() {
  struct node_pair *nodes;
  nodes = calloc(1, sizeof(struct node_pair));
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[32];
  char node2_to_1[32];
  while (1) {
    logger(MSG_ERROR, "%s: Initialize GPS proxy thread.\n", __func__);

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
  logger(MSG_ERROR, "%s: Initialize RMNET proxy thread.\n", __func__);
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
           nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
           nodes->node2.name);
  while (!nodes->allow_exit) {
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(nodes->node1.fd, &readfds);
    FD_SET(nodes->node2.fd, &readfds);
    pret = select(MAX_FD, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(nodes->node1.fd, &readfds)) {
      ret = read(nodes->node1.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        handle_call_pkt(buf, FROM_HOST, ret);
        track_client_count(buf, FROM_HOST, ret, nodes->node2.fd, nodes->node1.fd);
        dump_packet(node1_to_2, buf, ret);
        ret = write(nodes->node2.fd, buf, ret);
      }else {
          logger(MSG_ERROR, "%s: Closed descriptor at the ADSP side \n",
                 __func__);
        }
    } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
      ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        handle_call_pkt(buf, FROM_DSP, ret);
        track_client_count(buf, FROM_DSP, ret, nodes->node2.fd, nodes->node1.fd);
        dump_packet(node2_to_1, buf, ret);
        ret = write(nodes->node1.fd, buf, ret);
      }else {
          logger(MSG_ERROR, "%s: Closed descriptor at the USB side \n",
                 __func__);
        }
    }
  }
  logger(MSG_ERROR, "RMNET proxy is dead!.\n");
  close(nodes->node1.fd);
  close(nodes->node2.fd);
  return NULL;
}