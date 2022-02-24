// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "../inc/adspfw.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/md5sum.h"
#include "../inc/openqti.h"
#include "../inc/proxy.h"
#include "../inc/sms.h"
struct {
  bool adb_enabled;
  bool is_sms_notification_pending;
} atfwd_runtime_state;

void set_atfwd_runtime_default() {
  atfwd_runtime_state.adb_enabled = false;
  atfwd_runtime_state.is_sms_notification_pending = false;
}

void set_sms_notification_pending_state(bool en) {
  atfwd_runtime_state.is_sms_notification_pending = en;
}

bool get_sms_notification_pending_state() {
  return atfwd_runtime_state.is_sms_notification_pending;
}

void set_adb_runtime(bool mode) { atfwd_runtime_state.adb_enabled = mode; }

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

int send_pkt(struct qmi_device *qmidev, struct at_command_respnse *pkt,
             int sz) {
  pkt->qmipkt.length =
      htole16(sz - sizeof(struct qmi_packet)); // QMI packet size
  pkt->meta.at_pkt_len =
      htole16(sz - sizeof(struct qmi_packet) -
              sizeof(struct at_command_meta)); // AT packet size
  return sendto(qmidev->fd, pkt, sz, MSG_DONTWAIT, (void *)&qmidev->socket,
                sizeof(qmidev->socket));
}

int read_adsp_version() {
  char *md5_result;
  char *hex_md5_res;
  int ret, i, j;
  int element = -1;
  int offset = 0;
  md5_result = calloc(64, sizeof(char));
  hex_md5_res = calloc(64, sizeof(char));
  bool matched = false;

  ret = md5_file(ADSPFW_GEN_FILE, md5_result);
  if (strlen(md5_result) < 16) {
    logger(MSG_ERROR, "%s: Error calculating the MD5 for your firmware (%s) \n",
           __func__, md5_result);
  } else {
    for (j = 0; j < strlen(md5_result); j++) {
      offset += sprintf(hex_md5_res + offset, "%02x", md5_result[j]);
    }
    for (i = 0; i < (sizeof(known_adsp_fw) / sizeof(known_adsp_fw[0])); i++) {
      if (strcmp(hex_md5_res, known_adsp_fw[i].md5sum) == 0) {
        logger(MSG_INFO, "%s: Found your ADSP firmware: (%s) \n", __func__,
               known_adsp_fw[i].fwstring);
        element = i;
        matched = true;
        break;
      }
    }
  }
  if (!matched) {
    logger(MSG_WARN, "%s: Could not detect your ADSP firmware! \n", __func__);
  }
  free(md5_result);
  free(hex_md5_res);
  return element;
}

/* When a command is requested via AT interface,
   this function is used to answer to them */
int handle_atfwd_response(struct qmi_device *qmidev, uint8_t *buf,
                          unsigned int sz) {
  int j, sckret, ret;
  int packet_size;
  uint8_t cmdsize;
  char *parsedcmd;
  int cmd_id = -1;
  struct at_command_respnse *response;
  int pkt_size;
  int bytes_in_reply = 0;
  char filebuff[MAX_REPLY_SZ];
  FILE *fp;
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
  parsedcmd = calloc(cmdsize + 1, sizeof(char));
  strncpy(parsedcmd, (char *)buf + 19, cmdsize);

  logger(MSG_INFO, "%s: AT CMD: %s\n", __func__, parsedcmd);
  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    if (strcmp(parsedcmd, at_commands[j].cmd) == 0) {
      cmd_id = at_commands[j].command_id; // Command matched
    }
  }
  free(parsedcmd);

  /* Build initial response data */
  response = calloc(1, sizeof(struct at_command_respnse));
  response->qmipkt.ctlid = 0x00;
  response->qmipkt.transaction_id = htole16(qmidev->transaction_id);
  response->qmipkt.msgid = AT_CMD_RES;

  response->meta.client_handle = 0x01; // 0x01000801;
  response->handle = 0x0000000b;
  response->result = 1;   // result OK
  response->response = 3; // completed

  /* Set default sizes for the response packet
   *  If we don't write an extended response, the char array will be empty
   */
  pkt_size =
      sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);
  switch (cmd_id) {
  case 111: // QCPowerdown
    sckret = send_pkt(qmidev, response, pkt_size);
    usleep(500);
    reboot(0x4321fedc);
    break;
  case 109:
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 112: // ADB ON
    sckret = send_pkt(qmidev, response, pkt_size);
    store_adb_setting(true);
    restart_usb_stack();
    break;
  case 113: // ADB OFF // First respond to avoid locking the AT IF
    sckret = send_pkt(qmidev, response, pkt_size);
    store_adb_setting(false);
    restart_usb_stack();
    break;
  case 114: // Reset usb
    sckret = send_pkt(qmidev, response, pkt_size);
    reset_usb_port();
    break;
  case 115: // Reboot to recovery
    sckret = send_pkt(qmidev, response, pkt_size);
    set_next_fastboot_mode(1);
    reboot(0x01234567);
    break;
  case 116: // Is custom? alwas answer yes
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 117: // Enable PCM16k
    sckret = send_pkt(qmidev, response, pkt_size);
    set_auxpcm_sampling_rate(1);
    break;
  case 118: // Enable PCM48K
    sckret = send_pkt(qmidev, response, pkt_size);
    set_auxpcm_sampling_rate(2);
    break;
  case 119: // Enable PCM8K (disable pcmhi)
    sckret = send_pkt(qmidev, response, pkt_size);
    set_auxpcm_sampling_rate(0);
    break;
  case 120: // Fallback to 8K and enable USB_AUDIO
    sckret = send_pkt(qmidev, response, pkt_size);
    store_audio_output_mode(AUDIO_MODE_USB);
    restart_usb_stack();
    break;
  case 121: // Fallback to 8K and go back to I2S
    sckret = send_pkt(qmidev, response, pkt_size);
    store_audio_output_mode(AUDIO_MODE_I2S);
    restart_usb_stack();
    break;
  case 122: // Mute audio while in call
    sckret = send_pkt(qmidev, response, pkt_size);
    if (buf[30] == 0x31) {
      set_audio_mute(true);
    } else {
      set_audio_mute(false);
    }
    break;
  case 123: // Gracefully restart
    if (system("reboot") != 0) {
      response->result = 2;
    }
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 124: // Custom alert tone ON
    sckret = send_pkt(qmidev, response, pkt_size);
    set_custom_alert_tone(true);       // save to flash
    configure_custom_alert_tone(true); // enable in runtime
    break;
  case 125: // Custom alert tone off
    sckret = send_pkt(qmidev, response, pkt_size);
    set_custom_alert_tone(false);       // save to flash
    configure_custom_alert_tone(false); // runtime
    break;
  case 129: // QGMR
  case 126: // GETSWREV
    bytes_in_reply = sprintf(response->reply, "\r\n%s\r\n",
                             RELEASE_VER); // Either you add a line break or it
                                           // overwrites current line
    response->replysz = htole16(bytes_in_reply); // Size of the string to reply
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 127: // Fastboot
    sckret = send_pkt(qmidev, response, pkt_size);
    set_next_fastboot_mode(0);
    usleep(
        300); // Give it some time to be able to reach the ADSP before rebooting
    reboot(0x01234567);
    break;
  case 128: // GETFWBRANCH
    bytes_in_reply = sprintf(response->reply, "\r\ncommunity\r\n");
    response->replysz = htole16(bytes_in_reply);
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 130: // DMESG
    fp = fopen("/var/log/messages", "r");
    if (fp == NULL) {
      logger(MSG_ERROR, "%s: Error opening file \n", __func__);
      response->result = 2;
      sckret = send_pkt(qmidev, response, pkt_size);
    } else {
      bytes_in_reply = sprintf(response->reply, "\r\n");
      response->response = 0;
      response->replysz = htole16(bytes_in_reply);
      pkt_size = sizeof(struct at_command_respnse) -
                 (MAX_REPLY_SZ -
                  bytes_in_reply); // total size - (max string - used string)
      sckret = send_pkt(qmidev, response, pkt_size);
      ret = ftell(fp);
      if (ret > (MAX_REPLY_SZ * MAX_RESPONSE_NUM)) {
        fseek(fp, (ret - (MAX_REPLY_SZ * MAX_RESPONSE_NUM)), SEEK_SET);
      } else {
        fseek(fp, 0L, SEEK_SET);
      }
      do {
        memset(response->reply, 0, MAX_REPLY_SZ);
        ret = fread(filebuff, 1, MAX_REPLY_SZ - 2, fp);
        if (ret > 0) {
          response->response = 2;
          for (j = 0; j < ret - 1; j++) {
            if (filebuff[j] == 0x00) {
              filebuff[j] = ' ';
            }
          }
          strncpy(response->reply, filebuff, MAX_REPLY_SZ);
          response->replysz = htole16(ret);
          pkt_size =
              sizeof(struct at_command_respnse) -
              (MAX_REPLY_SZ - ret); // total size - (max string - used string)
          sckret = send_pkt(qmidev, response, pkt_size);
          usleep(500);
        }
      } while (ret > 0);
      fclose(fp);
      bytes_in_reply = sprintf(response->reply, "\r\nEOF\r\n");
      response->response = 1;
      response->replysz = htole16(bytes_in_reply);
      pkt_size = sizeof(struct at_command_respnse) -
                 (MAX_REPLY_SZ -
                  bytes_in_reply); // total size - (max string - used string)
      sckret = send_pkt(qmidev, response, pkt_size);
    }
    break;
  case 131: // OPENQTI LOG
    fp = fopen("/var/log/openqti.log", "r");
    if (fp == NULL) {
      logger(MSG_ERROR, "%s: Error opening file \n", __func__);
      response->result = 2;
      sckret = send_pkt(qmidev, response, pkt_size);
    } else {
      bytes_in_reply = sprintf(response->reply, "\r\n");
      response->response = 0;
      response->replysz = htole16(bytes_in_reply);
      pkt_size = sizeof(struct at_command_respnse) -
                 (MAX_REPLY_SZ -
                  bytes_in_reply); // total size - (max string - used string)
      sckret = send_pkt(qmidev, response, pkt_size);
      fseek(fp, 0L, SEEK_END);
      ret = ftell(fp);
      if (ret > (MAX_REPLY_SZ * MAX_RESPONSE_NUM)) {
        fseek(fp, (ret - (MAX_REPLY_SZ * MAX_RESPONSE_NUM)), SEEK_SET);
      } else {
        fseek(fp, 0L, SEEK_SET);
      }
      do {
        memset(response->reply, 0, MAX_REPLY_SZ);
        ret = fread(filebuff, 1, MAX_REPLY_SZ - 2, fp);
        if (ret > 0) {
          response->response = 2;
          strncpy(response->reply, filebuff, MAX_REPLY_SZ);
          response->replysz = htole16(ret);
          pkt_size =
              sizeof(struct at_command_respnse) -
              (MAX_REPLY_SZ - ret); // total size - (max string - used string)
          sckret = send_pkt(qmidev, response, pkt_size);
          usleep(500);
        }
      } while (ret > 0);
      fclose(fp);
      bytes_in_reply = sprintf(response->reply, "\r\nEOF\r\n");
      response->response = 1;
      response->replysz = htole16(bytes_in_reply);
      pkt_size = sizeof(struct at_command_respnse) -
                 (MAX_REPLY_SZ -
                  bytes_in_reply); // total size - (max string - used string)
      sckret = send_pkt(qmidev, response, pkt_size);
    }
    break;
  case 132: // Runtime set log level to debug
    sckret = send_pkt(qmidev, response, pkt_size);
    set_log_level(0);
    break;
  case 133: // Runtime set log level to info
    sckret = send_pkt(qmidev, response, pkt_size);
    set_log_level(1);
    break;
  case 134: // Simulate SMS
    bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",0\r\n");
    response->replysz = htole16(bytes_in_reply); // Size of the string to reply
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    inject_message(0);
    pulse_ring_in();
    break;
  case 135: // Simulate SMS Notification
    bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",0\r\n");
    response->replysz = htole16(bytes_in_reply);
    pkt_size =
        sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);
    sckret = send_pkt(qmidev, response, pkt_size);
    set_pending_notification_source(MSG_EXTERNAL);
    set_notif_pending(true);
    pulse_ring_in();
    //+CMTI: "ME",0
    break;
  case 136:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_suspend_inhibit(false);
    break;
  case 137:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_suspend_inhibit(true);
    break;
  case 138: // GETADSPVER
    j = read_adsp_version();
    if (j >= 0) {
      bytes_in_reply =
          sprintf(response->reply, "\r\n%s\r\n", known_adsp_fw[j].fwstring);
    } else {
      bytes_in_reply =
          sprintf(response->reply, "\r\nUnknown ADSP Firmware\r\n");
    }
    response->replysz = htole16(bytes_in_reply);
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 139:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_external_codec_defaults();
    break;
  case 140:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_record(true);
    break;
  case 141:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_record(false);
    break;
  default:
    // Fallback for dummy commands that arent implemented
    if ((cmd_id > 0 && cmd_id && cmd_id < 111)) {
      logger(MSG_INFO, "%s: Dummy command requested \n", __func__);
      response->result = 1;
    } else {
      logger(MSG_ERROR, "%s: Unknown command requested \n", __func__);
      response->result = 2;
    }
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  }

  qmidev->transaction_id++;
  free(response);
  response = NULL;
  return 0;
}

/* Send an URC indicating a new message and trigger the same
 * via QMI
 */
int at_send_cmti_urc(struct qmi_device *qmidev) {
  int sckret;
  struct at_command_respnse *response;
  int pkt_size;
  int bytes_in_reply = 0;

  /* Build initial response data */
  response = calloc(1, sizeof(struct at_command_respnse));
  response->qmipkt.ctlid = 0x00;
  response->qmipkt.transaction_id = htole16(qmidev->transaction_id);
  response->qmipkt.msgid = AT_CMD_RES;

  response->meta.client_handle = 0x01; // 0x01000801;
  response->handle = 0x0000000b;
  response->result = 1;   // result OK
  response->response = 3; // completed

  /* Set default sizes for the response packet
   *  If we don't write an extended response, the char array will be empty
   */
  pkt_size =
      sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);

  bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",0\r\n");
  response->replysz = htole16(bytes_in_reply);
  pkt_size =
      sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);
  sckret = send_pkt(qmidev, response, pkt_size);
  set_pending_notification_source(MSG_EXTERNAL);
  set_notif_pending(true);

  qmidev->transaction_id++;
  free(response);
  response = NULL;
  return 0;
}

/* Register AT commands into the DSP */
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
  struct timeval tv;
  at_qmi_dev = calloc(1, sizeof(struct qmi_device));
  logger(MSG_DEBUG, "%s: Initialize AT forwarding thread.\n", __func__);
  ret = init_atfwd(at_qmi_dev);
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Error setting up ATFWD!\n", __func__);
  }

  read_adsp_version();
  while (1) {

    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(at_qmi_dev->fd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(at_qmi_dev->fd, &readfds)) {
      ret = read(at_qmi_dev->fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        dump_packet("ATPort --> OpenQTI", buf, ret);
        handle_atfwd_response(at_qmi_dev, buf, ret);
      }
    }

    if (get_sms_notification_pending_state()) {
      logger(MSG_DEBUG, "%s: We just woke up, send CMTI\n", __func__);
      at_send_cmti_urc(at_qmi_dev);
      set_sms_notification_pending_state(false);
    }
  }
  // Close AT socket
  close(at_qmi_dev->fd);
  free(at_qmi_dev);
}
