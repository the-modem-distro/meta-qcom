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
      sleep(2);
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
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name,
           nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name,
           nodes->node2.name);
  while (1) {
  logger(MSG_ERROR, "%s: Initialize RMNET proxy thread.\n", __func__);
    if (nodes->node1.fd < 0) {
      nodes->node1.fd = open(RMNET_CTL, O_RDWR);
      if (nodes->node1.fd < 0) {
        logger(MSG_ERROR, "Error opening %s \n", RMNET_CTL);
      }
      if (nodes->node2.fd < 0) {
        nodes->node2.fd = open(SMD_CNTL, O_RDWR);
        if (nodes->node2.fd < 0) {
          logger(MSG_ERROR, "Error opening %s \n", SMD_CNTL);
        }
      }
    }
    if (nodes->node1.fd >= 0 && nodes->node2.fd >= 0) {
      nodes->allow_exit = false;
    } else {
      logger(MSG_ERROR, "One of the descriptors isn't ready\n");
      nodes->allow_exit = true;
      sleep(2);
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
          handle_call_pkt(buf, FROM_HOST, ret);
          if (track_client_count(buf, FROM_HOST, ret)) {
            force_close_qmi(nodes->node2.fd);
          }
          dump_packet(node1_to_2, buf, ret);
          ret = write(nodes->node2.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closing descriptor at the ADSP side \n",
                 __func__);
       //   nodes->allow_exit = true;
        }
      } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
        ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
        if (ret > 0) {
          handle_call_pkt(buf, FROM_DSP, ret);
          if (track_client_count(buf, FROM_DSP, ret)) {
            force_close_qmi(nodes->node2.fd);
          }
          dump_packet(node2_to_1, buf, ret);
          ret = write(nodes->node1.fd, buf, ret);
        } else {
          logger(MSG_ERROR, "%s: Closing descriptor at the USB side \n",
                 __func__);
         // nodes->allow_exit = true;
        }
      }
    }
    logger(MSG_ERROR, "We got out of the loop, restarting now.\n");
    close(nodes->node1.fd);
    close(nodes->node2.fd);

  }
}

uint32_t get_curr_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  uint32_t milliseconds =
      te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
  // printf("milliseconds: %lld\n", milliseconds);
  return milliseconds;
}