// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/tracking.h"

struct {
  struct service_pair services[32];
  uint8_t last_active;
  uint32_t regtime;
  uint8_t host_side_managing_app;
} client_tracking;

void reset_client_handler() {
  client_tracking.last_active = 0;
  client_tracking.regtime = 0;
  client_tracking.host_side_managing_app = 0;
  for (int i = 0; i < 32; i++) {
    client_tracking.services[i].service = 0;
    client_tracking.services[i].instance = 0;
  }
}

int get_num_instances_for_service(int service) {
  int i;
  int svcs = 0;
  for (i = 0; i < 32; i++) {
    if (service == client_tracking.services[i].service) {
      svcs++;
    }
  }
  return svcs;
}

bool is_register_release_request(uint8_t *pkt, int sz) {
  if (pkt[0] == 0x01 && pkt[9] == 0x00 && sz >= 12) {
    if (pkt[8] == CLIENT_REGISTER_REQ || pkt[8] == CLIENT_RELEASE_REQ) {
      return true;
    }
  }
  return false;
}

int add_client(uint8_t service, uint8_t instance) {
  client_tracking.services[client_tracking.last_active].service = service;
  client_tracking.services[client_tracking.last_active].instance = instance;
  client_tracking.last_active++;
  client_tracking.regtime = get_curr_timestamp();
  if (client_tracking.last_active > 16) {
    return -ENOSPC;
  }
  return 0;
}

int remove_client(uint8_t service, uint8_t instance) {
  int i;
  for (i = 32; i >= 0; i--) {
    if (client_tracking.services[i].service == service &&
        client_tracking.services[i].instance == instance) {
      client_tracking.services[i].service = 0;
      client_tracking.services[i].instance = 0;
      client_tracking.last_active--;
      if (client_tracking.last_active <= 0) {
        logger(MSG_INFO, "%s: All QMI Clients have been freed from the host\n",
               __func__);
        reset_client_handler();
        return 1;
      } else {
        client_tracking.regtime = get_curr_timestamp();
        return 0;
      }
    }
  }
  return -ENOENT;
}

int set_current_host_app(uint8_t app) {
  if (app == HOST_USES_MODEMMANAGER || app == HOST_USES_OFONO) {
    client_tracking.host_side_managing_app = app;
    logger(MSG_INFO, "%s: Host app is 0x%2x \n", __func__, app);
    return 0;
  }
  logger(MSG_ERROR, "%s: Host app is unknown: 0x.%2x \n", __func__, app);
  return -EINVAL;
}

void update_register_time() { client_tracking.regtime = get_curr_timestamp(); }

uint8_t get_current_host_app() {
  return client_tracking.host_side_managing_app;
}

int track_client_count(uint8_t *pkt, int from, int sz, int fd, int rmnet_fd) {
  int msglength, ret;
  if (!is_register_release_request(pkt, sz)) {
    return 0;
  }
  if (sz > 20) {
    msglength = pkt[10] + 10;
  }

  /* The seven states of denial
   *
   * 1. This is the first time someone is trying to connect
   * 2. This is a normal part of the connection attempt
   * 3. Someone is trying to connect again without releasing itself
   * 4. Someone is trying to connect again when it was in the middle of
   * releasing itself
   * 5. Someone was trying to connect and it was cut off in the middle (god I
   * hate usb)
   * 6. ???
   * 7. Someone is just trying to deregister itself
   * It seems there could be an unhandled case here (needs more debugging) where
   * ModemManager was force closed, is reregistering but gets disconnected again
   * If this happens you'll see a SIM with a question mark in Phosh
   */
  if (pkt[8] == CLIENT_REGISTER_REQ && pkt[10] == 0x04 && from == FROM_HOST) {
    logger(MSG_INFO, "%s: Request for service 0x%.2x...\n", __func__, pkt[15]);
    // 1. This is the first time someone is trying to connect
    if (client_tracking.regtime == 0) {
      update_register_time();
      if (client_tracking.host_side_managing_app == 0) {
        ret = set_current_host_app(pkt[15]);
      }
    } else if (sz >= 15 && get_current_host_app() == pkt[15] ==
               client_tracking.last_active > 0) {
      // We'd hit this if USB vanishes during suspend
      logger(MSG_WARN, "%s: Dirty re-register attempt from host \n", __func__);
      force_close_qmi(fd);
      send_rmnet_ioctls(rmnet_fd);
    } else if (client_tracking.last_active > MAX_ACTIVE_CLIENTS) {
      // We'd hit this if host app dies mid reconnect more than once
      logger(MSG_WARN, "%s: Too many clients, resetting... \n", __func__);
      force_close_qmi(fd);
      reset_usb_port();
      send_rmnet_ioctls(rmnet_fd);
    }
  } else if (sz >= 16 && pkt[8] == CLIENT_REGISTER_REQ && pkt[10] == 0x05 &&
             from == FROM_HOST) {
    logger(MSG_INFO, "%s: Register request: S:0x%.2x I:0x%.2x\n", __func__,
           pkt[15], pkt[16]);
    client_tracking.regtime = get_curr_timestamp();
  } else if (pkt[8] == CLIENT_REGISTER_REQ && from == FROM_DSP) {
    logger(MSG_INFO, "%s: DSP: Add client: S:0x%.2x I:0x%.2x, AC:%i \n",
           __func__, pkt[msglength], pkt[msglength + 1],
           client_tracking.last_active);
    ret = add_client(pkt[msglength], pkt[msglength + 1]);
  } else if (sz >= 16 && pkt[8] == CLIENT_RELEASE_REQ && from == FROM_DSP) {
    logger(MSG_INFO, "%s: QMI Client Release from HOST,S:%.2x I:%.2x, AC:%i \n",
           __func__, pkt[15], pkt[16], client_tracking.last_active);
    ret = remove_client(pkt[15], pkt[16]);
  }

  return 0;
}

void send_rmnet_ioctls(int fd) {
  int ret, linestate;
  ret = ioctl(fd, GET_LINE_STATE, &linestate);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Error getting line state  %i, %i \n", __func__,
           linestate, ret);

  // Set modem OFFLINE AND ONLINE
  ret = ioctl(fd, MODEM_OFFLINE);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Set modem offline: %i \n", __func__, ret);

  ret = ioctl(fd, MODEM_ONLINE);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Set modem online: %i \n", __func__, ret);
}
/*
 * This function will loop through all services, connected or not
 * and will send a release request. It doesn't matter if it fails
 * in some service because there was nothing attached to it, we
 * just need to drain the ADSP, so if there's something we didn't
 * catch before we'll kill it anyway here.
 */
void force_close_qmi(int fd) {
  int transaction_id = 0;
  int ret, i, instance, service;
  uint8_t release_prototype[] = {0x01, 0x10, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x04, 0x23, 0x00, 0x05, 0x00,
                                 0x01, 0x02, 0x00, 0x1a, 0x01};
  /* TEST */
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  bool is_drained = false;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  /* TEST */
  logger(MSG_WARN, "%s: Wiping any registered client to any instance\n",
         __func__);
  for (service = 0xff; service >= 0x00; service--) {
    for (instance = 0; instance <= 0x05; instance++) {
      release_prototype[7] = transaction_id;
      release_prototype[15] = service;
      release_prototype[16] = instance;
      ret = write(fd, release_prototype, sizeof(release_prototype));
      logger(MSG_DEBUG,
             "%s: Closing connection to service %.2x, instance %i, bytes "
             "written: %i \n",
             __func__, client_tracking.services[i], instance, ret);
      transaction_id++;
    }
  }

  while (!is_drained) {
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(fd, &readfds);
    ret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(fd, &readfds)) {
      ret = read(fd, &buf, MAX_PACKET_SIZE); // and don't do anything with it
    } else {
      is_drained = true;
    }
  }
  client_tracking.regtime = 0;
  client_tracking.last_active = 0;
  for (i = 0; i < 32; i++) {
    client_tracking.services[i].service = 0;
    client_tracking.services[i].instance = 0;
  }

  sleep(1);
}