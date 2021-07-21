// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "../inc/helpers.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
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

void *two_way_proxy(void *node_data) {
  struct node_pair *nodes = (struct node_pair *)node_data;
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  char node1_to_2[32];
  char node2_to_1[32];
  logger(MSG_DEBUG, "%s: Initialize two way proxy thread.\n", __func__);
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name, nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name, nodes->node2.name);

  while (1) {
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
      }
    } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
      ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        dump_packet(node2_to_1, buf, ret);
        ret = write(nodes->node1.fd, buf, ret);
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
  char node1_to_2[32];
  char node2_to_1[32];
  logger(MSG_DEBUG, "%s: Initialize RMNET proxy thread.\n", __func__);
  snprintf(node1_to_2, sizeof(node1_to_2), "%s-->%s", nodes->node1.name, nodes->node2.name);
  snprintf(node2_to_1, sizeof(node2_to_1), "%s<--%s", nodes->node1.name, nodes->node2.name);

  while (looped) {
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
          looped = false;
        }
        dump_packet(node1_to_2, buf, ret);
        ret = write(nodes->node2.fd, buf, ret);
      }
    } else if (FD_ISSET(nodes->node2.fd, &readfds)) {
      ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        handle_call_pkt(buf, FROM_DSP, ret);
        if (track_client_count(buf, FROM_DSP, ret)) {
           force_close_qmi(nodes->node2.fd);
            looped = false;
        }
        dump_packet(node2_to_1, buf, ret);
        ret = write(nodes->node1.fd, buf, ret);
      }
    }
  }
  pthread_exit((void *) 0);
}

uint32_t get_curr_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    uint32_t milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}