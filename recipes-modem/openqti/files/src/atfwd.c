// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
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

/* Used to keep track of the enabled USB composite configuration */
int current_usb_mode = 0;

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

void switch_adb(bool en) {
  if (en == true && (current_usb_mode == 0 || current_usb_mode == 2)) {
    logger(MSG_ERROR, "%s: ADB is already enabled \n", __func__);
    return;
  }
  if (en == false && (current_usb_mode == 1 || current_usb_mode == 3)) {
    logger(MSG_ERROR, "%s: ADB is already enabled \n", __func__);
    return;
  }
  /* First we shut down the usb port to be able to modify Sysfs */
  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }

  if (write_to(USB_SERIAL_TRANSPORTS_PATH, usb_modes[0].serial_transports,
               O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error setting serial transports \n", __func__);
  }
  if (en) {
    // setup ADB
    if (current_usb_mode == 1 &&
        write_to(USB_FUNC_PATH, usb_modes[0].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error enabling ADB functions \n", __func__);
    } else if (current_usb_mode == 3 &&
               write_to(USB_FUNC_PATH, usb_modes[2].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error enabling ADB+AUDIO functions \n", __func__);
    }

    system("/etc/init.d/adbd start");
    current_usb_mode--; // ADB ON profiles are always one less than the "off"
                        // ones
  } else {
    // setup ADB
    system("/etc/init.d/adbd stop");
    if (current_usb_mode == 0 &&
        write_to(USB_FUNC_PATH, usb_modes[1].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error disabling ADB functions \n", __func__);
    } else if (current_usb_mode == 2 &&
               write_to(USB_FUNC_PATH, usb_modes[3].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error disabling ADB+AUDIO functions \n", __func__);
    }

    current_usb_mode++; // ADB OFF profiles are always one more
  }

  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

void switch_usb_audio(bool en) {
  if (en == true && (current_usb_mode == 2 || current_usb_mode == 3)) {
    logger(MSG_ERROR, "%s: USB Audio is already enabled \n", __func__);
    return;
  }
  if (en == false && (current_usb_mode == 0 || current_usb_mode == 1)) {
    logger(MSG_ERROR, "%s: USB Audio is already disabled \n", __func__);
    return;
  }
  /* First we shut down the usb port to be able to modify Sysfs */
  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }

  if (en) {
    // setup USB audio
    if (current_usb_mode == 0 &&
        write_to(USB_FUNC_PATH, usb_modes[2].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error enabling ADB+AUDIO functions \n", __func__);
    } else if (current_usb_mode == 1 &&
               write_to(USB_FUNC_PATH, usb_modes[3].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error enabling AUDIO functions \n", __func__);
    }

    current_usb_mode = current_usb_mode + 2; // Audio profile is +2
  } else {
    // Disable USB audio
    if (current_usb_mode == 2 &&
        write_to(USB_FUNC_PATH, usb_modes[0].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error disabling ADB functions \n", __func__);
    } else if (current_usb_mode == 3 &&
               write_to(USB_FUNC_PATH, usb_modes[1].functions, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error disabling ADB+AUDIO functions \n", __func__);
    }

    current_usb_mode = current_usb_mode - 2; // USB Audio off profiles are -2
  }

  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}
// AT+QDAI=1,1,0,1,0,0,1,1
int set_audio_profile(uint8_t io, uint8_t mode, uint8_t fsync, uint8_t clock,
                      uint8_t format, uint8_t sample, uint8_t num_slots,
                      uint8_t slot) {
  uint8_t ret = 0;
  return ret;
  /* We know we don't have an analog codec here,
     so EINVAL if anything other than Digital PCM (i2s) */
  logger(MSG_ERROR,
         "Set audio profile: 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x "
         "0x%.2x\n",
         io, mode, fsync, clock, format, sample, num_slots, slot);
  switch (io) { // 1,2,3,5, Digital PCM, <--analog-->
  case 0x31:
    if (write_to(sysfs_value_pairs[0].path, "0", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[0].path);
      ret = -EINVAL;
    }
    break;
  default:
    ret = -EINVAL;
    break;
  }
  switch (mode) { // Master / Slave
  case 0x30:      // Master (they're flipped?)
    if (write_to(sysfs_value_pairs[2].path, "1", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[1].path);
      ret = -EINVAL;
    }
    break;
  case 0x31: // Slave
    if (write_to(sysfs_value_pairs[2].path, "0", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[1].path);
      ret = -EINVAL;
    }
    break;
  default:
    ret = -EINVAL;
    break;
  }
  switch (fsync) { // Short sync / Long sync
  case 0x30:
    if (write_to(sysfs_value_pairs[1].path, "0", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[1].path);
      ret = -EINVAL;
    }
    break;
  case 0x31:
    if (write_to(sysfs_value_pairs[1].path, "1", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[1].path);
      ret = -EINVAL;
    }
    break;
  default:
    return -EINVAL;
    break;
  }
  switch (clock) { // Clock freq
  case 0x30:       // 128
    if (write_to(sysfs_value_pairs[5].path, "128000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  case 0x31: // 256
    if (write_to(sysfs_value_pairs[5].path, "256000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  case 0x32: // 512
    if (write_to(sysfs_value_pairs[5].path, "512000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  case 0x33: // 1024
    if (write_to(sysfs_value_pairs[5].path, "1024000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  case 0x34: // 2048
    if (write_to(sysfs_value_pairs[5].path, "2048000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  case 0x35: // 4096
    if (write_to(sysfs_value_pairs[5].path, "4096000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[5].path);
      ret = -EINVAL;
    }
    break;
  default:
    ret = -EINVAL;
    break;
  }
  switch (format) {
  case 0x30: // 16bit linear
  case 0x31: // 8bit a-law
  case 0x32: // 8bit u-law
    break;
  default:
    ret = -EINVAL;
    break;
  }
  switch (sample) { // Sample rate
  case 0x30:        // 8k
    if (write_to(sysfs_value_pairs[6].path, "8000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[6].path);
      ret = -EINVAL;
    }
    // Write bits per frame too. Anything less than 5 for 8k
    if (write_to(sysfs_value_pairs[3].path, "2", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[3].path);
      ret = -EINVAL;
    }
    break;
  case 0x31: // 16k
    if (write_to(sysfs_value_pairs[6].path, "16000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[6].path);
      ret = -EINVAL;
    }
    /* Per msm-dai-q6-v2.c, use frame > AFE_PORT_PCM_BITS_PER_FRAME_256
       to specify PCM16k mode. Dont know if this works yet
    */
    if (write_to(sysfs_value_pairs[3].path, "6", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[3].path);
      ret = -EINVAL;
    }
    break;
  default:
    ret = -EINVAL;
    break;
  }
  /* Number of slots and current slot are not used */
  return ret;
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
    switch_adb(true);
    break;
  case 113: // ADB OFF // First respond to avoid locking the AT IF
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));
    store_adb_setting(false);
    switch_adb(false);
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

    switch_usb_audio(true);
    break;
  case 121: // Fallback to 8K and enable USB_AUDIO
    cmdreply->result = 1;
    sckret =
        sendto(qmidev->fd, cmdreply, sizeof(struct at_command_simple_reply),
               MSG_DONTWAIT, (void *)&qmidev->socket, sizeof(qmidev->socket));

    switch_usb_audio(false);
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

  if (!is_adb_enabled()) {
    current_usb_mode = 1;
  }
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
