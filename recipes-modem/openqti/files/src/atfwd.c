// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"

struct {
  bool adb_enabled;
} atfwd_runtime_state;

void set_atfwd_runtime_default() {
  atfwd_runtime_state.adb_enabled = false;
}

void set_adb_runtime(bool mode) {
  atfwd_runtime_state.adb_enabled = mode;
}

void build_atcommand_reg_request(int tid, const char *command, char *buf) {
  struct atcmd_reg_request *atcmd;
  int k, i;
  ssize_t cmdsize, bufsize;
  atcmd =
      (struct atcmd_reg_request *)calloc(1, sizeof(struct atcmd_reg_request));
  atcmd->ctlid = 0x00;
  atcmd->transaction_id = htole16(tid);
  atcmd->msgid = htole16(32);
  atcmd->dummy1 = 0x01;
  atcmd->dummy2 = 0x0100;
  atcmd->dummy3 = 0x00;
  cmdsize = strlen(command);
  atcmd->packet_size = strlen(command) + 40 +
                       sizeof(struct atcmd_reg_request); // atcmd + command + 36
  atcmd->command_length = strlen(command);
  atcmd->var2 = strlen(command) + 2; // It has to be something like this
  atcmd->var1 = atcmd->var2 + 3;
  bufsize = sizeof(struct atcmd_reg_request) +
            ((strlen(command) + 43) * sizeof(char));
  memcpy(buf, atcmd, sizeof(struct atcmd_reg_request));
  memcpy(buf + sizeof(struct atcmd_reg_request), command,
         (sizeof(char) * strlen(command)));
  free(atcmd);
  atcmd = NULL;
}


/* When a command is requested via AT interface,
   this function is used to answer to them */
int handle_atfwd_response(struct qmi_device *qmidev, uint8_t *buf,
                          unsigned int sz) {
  int i, j, sckret;
  int packet_size;
  uint8_t cmdsize, cmdend;
  char *parsedcmd;
  int cmd_id = -1;
  struct at_command_simple_reply *cmdreply;
  cmdreply = calloc(1, sizeof(struct at_command_simple_reply));
  if (sz == 14) {
    logger(MSG_DEBUG, "%s: Packet ACK\n", __func__);
    return 0;
  }
  if (sz < 16) {
    logger(MSG_ERROR, "Packet too small: %i\n", sz);
    return 0;
  }
  if (buf[0] != 0x04) {
    logger(MSG_ERROR, "Unknown control ID!\n");
    return 0;
  }
  if (buf[3] != 0x21) {
    logger(MSG_ERROR, "Unknown message type!\n");
    return 0;
  }

  packet_size = buf[5];
  logger(MSG_DEBUG, "Packet size is %i, received total size is %i\n",
         packet_size, sz);

  cmdsize = buf[18];
  cmdend = 18 + cmdsize;

  parsedcmd = calloc(cmdsize + 1, sizeof(char));
  strncpy(parsedcmd, (char *)buf + 19, cmdsize);

  logger(MSG_WARN, "%s: Command requested is %s\n", __func__, parsedcmd);
  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    if (strcmp(parsedcmd, at_commands[j].cmd) == 0) {
      cmd_id = at_commands[j].command_id; // Command matched
    }
  }
  free(parsedcmd);

  cmdreply->ctlid = 0x00;
  cmdreply->transaction_id = htole16(qmidev->transaction_id);
  cmdreply->msgid = 0x0022;

  cmdreply->length = htole16(sizeof(struct at_command_simple_reply) -
                             (3 * sizeof(uint16_t) - sizeof(uint8_t)) -
                             (2 * (sizeof(unsigned char))));
  cmdreply->handle = 0x01000801;
  cmdreply->response = 3;
  cmdreply->result = 1;

  switch (cmd_id) {
  case 40: // QDAI
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    break;
  case 72: // fastboot
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    set_next_fastboot_mode(0);
    reboot(0x01234567);
    break;
  case 111: // QCPowerdown
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    reboot(0x4321fedc);
    break;
  case 109:
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    break;
  case 112: // ADB ON
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    store_adb_setting(true);
    restart_usb_stack();
    break;
  case 113: // ADB OFF // First respond to avoid locking the AT IF
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    store_adb_setting(false);
    restart_usb_stack();
    break;
  case 114: // reset usb
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    reset_usb_port();
    break;
  case 115: // reboot to recovery
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    set_next_fastboot_mode(1);
    reboot(0x01234567);
    break;
  case 116:
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    break;
  case 117: // enable PCM16k
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    set_auxpcm_sampling_rate(1);
    break;
  case 118: // PCM48K
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));

    set_auxpcm_sampling_rate(2);
    break;
  case 119: // disable PCM HI EN_PCM8K
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));

    set_auxpcm_sampling_rate(0);
    break;
  case 120: // Fallback to 8K and enable USB_AUDIO
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    store_audio_output_mode(AUDIO_MODE_USB);
    restart_usb_stack();
    break;
  case 121: // Fallback to 8K and enable USB_AUDIO
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));

    store_audio_output_mode(AUDIO_MODE_I2S);
    restart_usb_stack();
    break;
  default:
    // Fallback for dummy commands that arent implemented
    if ((cmd_id > 0 && cmd_id < 72) || (cmd_id > 72 && cmd_id < 108)) {
      cmdreply->result = 1;
    } else {
      logger(MSG_ERROR, "%s: Unknown command requested \n", __func__);
      cmdreply->result = 2;
    }
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    break;
  }

  qmidev->transaction_id++;
  free(cmdreply);
  cmdreply = NULL;
  return 0;
}

/* Register AT commands into the DSP
   This is currently not working

*/
int init_atfwd(struct qmi_device *qmidev) {
  int i, j, ret;
  ssize_t pktsize;
  char *atcmd;
  struct timeval tv;
  int tmpstate = 0;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  bool state = false;
  socklen_t addrlen = sizeof(struct sockaddr_msm_ipc);
  char buf[MAX_PACKET_SIZE];
  uint32_t node_id, port_id;
  struct msm_ipc_server_info at_port;

  at_port = get_node_port(8, 0); // Get node port for AT Service, _any_ instance

  logger(MSG_DEBUG, "%s: Connecting to IPC... \n", __func__);
  ret = open_ipc_socket(qmidev, at_port.service, at_port.instance,
                        at_port.node_id, at_port.port_id,
                        IPC_ROUTER_AT_ADDRTYPE); // open ATFWD service
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Error opening socket \n", __func__);
    return -EINVAL;
  }
  ret = setsockopt(qmidev->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (ret) {
    logger(MSG_ERROR, "%s: Error setting socket options \n", __func__);
  }

  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    atcmd = calloc(256, sizeof(uint8_t));

    logger(MSG_DEBUG, "%s: --> CMD %i: %s ", __func__,
           at_commands[j].command_id, at_commands[j].cmd);
    build_atcommand_reg_request(qmidev->transaction_id, at_commands[j].cmd,
                                atcmd);
    qmidev->transaction_id++;

    pktsize = sizeof(struct atcmd_reg_request) +
              ((strlen(at_commands[j].cmd) + 47) * sizeof(char));
    ret = sendto(qmidev->fd, atcmd, pktsize, MSG_DONTWAIT,
                 (void *)&qmidev->socket, sizeof(qmidev->socket));
    if (ret < 0) {
      logger(MSG_DEBUG, "%s: Failed (%i) \n", __func__, ret);
    }
    ret = recvfrom(qmidev->fd, buf, sizeof(buf), 0,
                   (struct sockaddr *)&qmidev->socket, &addrlen);
    if (ret < 0) {
      logger(MSG_DEBUG, "%s: Sent but rejected (%i) \n", __func__, ret);
    }
    if (ret == 14) {
      if (buf[12] == 0x00) {
        logger(MSG_DEBUG, "%s: Command accepted (0x00) \n", __func__, ret);
      } else if (buf[12] == 0x01) {
        logger(MSG_DEBUG, "%s: Sent but rejected (0x01) \n", __func__, ret);
      }
      /* atfwd_daemon responds 0x30 on success, but 0x00 will do too.
        When it responds 0x02 or 0x01 byte 12 it always fail, no fucking clue
        why */
    } else {
      logger(MSG_ERROR, "%s: Unknown response from the DSP \n", __func__, ret);
    }
    free(atcmd);
  }
  atcmd = NULL;

  return 0;
}

void *start_atfwd_thread() {
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  struct qmi_device *at_qmi_dev;
  at_qmi_dev = calloc(1, sizeof(struct qmi_device));
  logger(MSG_DEBUG, "%s: Initialize AT forwarding thread.\n", __func__);
  ret = init_atfwd(at_qmi_dev);
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Error setting up ATFWD!\n", __func__);
  }

  while (1) {
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(at_qmi_dev->fd, &readfds);
    pret = select(MAX_FD, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(at_qmi_dev->fd, &readfds)) {
      ret = read(at_qmi_dev->fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        dump_packet("ATPort --> OpenQTI", buf, ret);
        handle_atfwd_response(at_qmi_dev, buf, ret);
      }
    }
  }
  // Close AT socket
  close(at_qmi_dev->fd);
  free(at_qmi_dev);
}
