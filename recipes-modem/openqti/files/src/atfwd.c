// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <syscall.h>
#include <unistd.h>

#include "../inc/adspfw.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/proxy.h"
#include "../inc/sms.h"
#include "../inc/nas.h"

struct {
  bool adb_enabled;
  bool is_sms_notification_pending;
  bool random_debug_cb;
  bool debug_cb;
  bool stream_cb;
} atfwd_runtime_state;

void set_atfwd_runtime_default() {
  atfwd_runtime_state.adb_enabled = false;
  atfwd_runtime_state.is_sms_notification_pending = false;
  atfwd_runtime_state.random_debug_cb = false;
  atfwd_runtime_state.debug_cb = false;
  atfwd_runtime_state.stream_cb = false;
}
bool at_debug_cb_message_requested() {
  return atfwd_runtime_state.debug_cb;
}

bool at_debug_random_cb_message_requested() {
  return atfwd_runtime_state.random_debug_cb;
}

bool at_debug_stream_cb_message_requested() {
  return atfwd_runtime_state.stream_cb;
}
void send_cb_message_to_modemmanager(int usbfd, int message_id) {
  int pktsz = 0;
  uint8_t *pkt;
  uint8_t example_pkt1[] = {0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x02, 0x00, 0x01, 0x00, 0x67, 0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x16, 0xc4, 0x64, 0x71, 0x0a, 0x4a, 0x4e, 0xa9, 0xa0, 0x62, 0xd2, 0x59, 0x04, 0x15, 0xa5, 0x50, 0xe9, 0x53, 0x58, 0x75, 0x1e, 0x15, 0xc4, 0xb4, 0x0b, 0x24, 0x93, 0xb9, 0x62, 0x31, 0x97, 0x0c, 0x26, 0x93, 0x81, 0x5a, 0xa0, 0x98, 0x4d, 0x47, 0x9b, 0x81, 0xaa, 0x68, 0x39, 0xa8, 0x05, 0x22, 0x86, 0xe7, 0x20, 0xa1, 0x70, 0x09, 0x2a, 0xcb, 0xe1, 0xf2, 0xb7, 0x98, 0x0e, 0x22, 0x87, 0xe7, 0x20, 0x77, 0xb9, 0x5e, 0x06, 0x21, 0xc3, 0x6e, 0x72, 0xbe, 0x75, 0x0d, 0xcb, 0xdd, 0xad, 0x69, 0x7e, 0x4e, 0x07, 0x16, 0x01, 0x00, 0x00 };
  uint8_t example_pkt2[] = {0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x03, 0x00, 0x01, 0x00, 0x67, 0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x26, 0xe5, 0x36, 0x28, 0xed, 0x06, 0x25, 0xd1, 0xf2, 0xb2, 0x1c, 0x24, 0x2d, 0x9f, 0xd3, 0x6f, 0xb7, 0x0b, 0x54, 0x9c, 0x83, 0xc4, 0xe5, 0x39, 0xbd, 0x8c, 0xa6, 0x83, 0xd6, 0xe5, 0xb4, 0xbb, 0x0c, 0x3a, 0x96, 0xcd, 0x61, 0xb4, 0xdc, 0x05, 0x12, 0xa6, 0xe9, 0xf4, 0x32, 0x28, 0x7d, 0x76, 0xbf, 0xe5, 0xe9, 0xb2, 0xbc, 0xec, 0x06, 0x4d, 0xd3, 0x65, 0x10, 0x39, 0x5d, 0x06, 0x39, 0xc3, 0x63, 0xb4, 0x3c, 0x3d, 0x46, 0xd3, 0x5d, 0xa0, 0x66, 0x19, 0x2d, 0x07, 0x25, 0xdd, 0xe6, 0xf7, 0x1c, 0x64, 0x06, 0x16, 0x01, 0x00, 0x00 };
  uint8_t example_pkt3[] = {0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x04, 0x00, 0x01, 0x00, 0x67, 0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x36, 0x69, 0x37, 0xb9, 0xec, 0x06, 0x4d, 0xd3, 0x65, 0x50, 0xdd, 0x4d, 0x2f, 0xcb, 0x41, 0x68, 0x3a, 0x1d, 0xae, 0x7b, 0xbd, 0xee, 0xf7, 0xbb, 0x4b, 0x2c, 0x5e, 0xbb, 0xc4, 0x75, 0x37, 0xd9, 0x45, 0x2e, 0xbf, 0xc6, 0x65, 0x36, 0x5b, 0x2c, 0x7f, 0x87, 0xc9, 0xe3, 0xf0, 0x9c, 0x0e, 0x12, 0x82, 0xa6, 0x6f, 0x36, 0x9b, 0x5e, 0x76, 0x83, 0xa6, 0xe9, 0x32, 0x88, 0x9c, 0x2e, 0xcf, 0xcb, 0x20, 0x67, 0x78, 0x8c, 0x96, 0xa7, 0xc7, 0x68, 0x3a, 0xa8, 0x2c, 0x47, 0x87, 0xd9, 0xf4, 0xb2, 0x9b, 0x05, 0x02, 0x16, 0x01, 0x00, 0x00 };
  uint8_t example_pkt4[] = {0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x67, 0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x46, 0xf3, 0x37, 0xc8, 0x5e, 0x96, 0x9b, 0xfd, 0x67, 0x3a, 0x28, 0x89, 0x96, 0x83, 0xa6, 0xed, 0xb0, 0x9c, 0x0e, 0x47, 0xbf, 0xdd, 0x65, 0xd0, 0xf9, 0x6c, 0x76, 0x81, 0xdc, 0xe9, 0x31, 0x9a, 0x0e, 0xf2, 0x8b, 0xcb, 0x72, 0x10, 0xb9, 0xec, 0x06, 0x85, 0xd7, 0xf4, 0x7a, 0x99, 0xcd, 0x2e, 0xbb, 0x41, 0xd3, 0xb7, 0x99, 0x7e, 0x0f, 0xcb, 0xcb, 0x73, 0x7a, 0xd8, 0x4d, 0x76, 0x81, 0xae, 0x69, 0x39, 0xa8, 0xdc, 0x86, 0x9b, 0xcb, 0x68, 0x76, 0xd9, 0x0d, 0x22, 0x87, 0xd1, 0x65, 0x39, 0x0b, 0x94, 0x04, 0x16, 0x01, 0x00, 0x00 };
  uint8_t example_pkt5[] = {0x01, 0x73, 0x00, 0x80, 0x05, 0x01, 0x04, 0x06, 0x00, 0x01, 0x00, 0x67, 0x00, 0x11, 0x60, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x58, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x56, 0x68, 0x39, 0xa8, 0xe8, 0x26, 0x9f, 0xcb, 0xf2, 0x3d, 0x1d, 0x14, 0xae, 0x9b, 0x41, 0xe4, 0xb2, 0x1b, 0x14, 0x5e, 0xd3, 0xeb, 0x65, 0x36, 0xbb, 0xec, 0x06, 0x4d, 0xdf, 0x66, 0xfa, 0x3d, 0x2c, 0x2f, 0xcf, 0xe9, 0x61, 0x37, 0x19, 0xa4, 0xaf, 0x83, 0xe0, 0x72, 0xbf, 0xb9, 0xec, 0x76, 0x81, 0x84, 0xe5, 0x34, 0xc8, 0x28, 0x0f, 0x9f, 0xcb, 0x6e, 0x10, 0x3a, 0x5d, 0x96, 0xeb, 0xeb, 0xa0, 0x7b, 0xd9, 0x4d, 0x2e, 0xbb, 0x41, 0xd3, 0x74, 0x19, 0x34, 0x4f, 0x8f, 0xd1, 0x20, 0x71, 0x9a, 0x4e, 0x07, 0x16, 0x01, 0x00, 0x00 };
  uint8_t example_pkt6[] = {0x01, 0x3a, 0x00, 0x80, 0x05, 0x01, 0x04, 0x07, 0x00, 0x01, 0x00, 0x2e, 0x00, 0x11, 0x27, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x1f, 0x00, 0x40, 0x00, 0x11, 0x1c, 0x00, 0x66, 0x65, 0x50, 0xd8, 0x0d, 0x4a, 0xa2, 0xe5, 0x65, 0x37, 0xe8, 0x58, 0x96, 0xef, 0xe9, 0x65, 0x74, 0x59, 0x3e, 0xa7, 0x97, 0xd9, 0xec, 0xb2, 0xdc, 0xd5, 0x16, 0x01, 0x00, 0x00 };
  srand(time(NULL));
  int rndnum = rand()%((5));

  /* Reset state */
  atfwd_runtime_state.random_debug_cb = false;
  atfwd_runtime_state.debug_cb = false;
  atfwd_runtime_state.stream_cb = false;

  if (message_id == -2) {
    logger(MSG_WARN, "Sending a multipart stream!");
    for (int i = 0; i < 6; i++) {
      switch(i) {
        case 0:
          pktsz = sizeof(example_pkt1);
          pkt = example_pkt1;
          break;
        case 1:
          pktsz = sizeof(example_pkt2);
          pkt = example_pkt2;
          break;
        case 2:
          pktsz = sizeof(example_pkt3);
          pkt = example_pkt3;
          break;
        case 3:
          pktsz = sizeof(example_pkt4);
          pkt = example_pkt4;
          break;
        case 4:
          pktsz = sizeof(example_pkt5);
          pkt = example_pkt5;
          break;
        case 5:
          pktsz = sizeof(example_pkt6);
          pkt = example_pkt6;
          break;     
      }
    if (write(usbfd, pkt, pktsz) < 0) {
      logger(MSG_ERROR, "%s: Error sending new CB message %i\n", __func__, i);
    } else {
      logger(MSG_INFO, "%s: Sent CB message %i\n", __func__, i);
    }
    logger(MSG_INFO, "Sleep 1 sec...");
    sleep(1);
    }
  } else if (message_id == -1) {
    logger(MSG_INFO, "Random message selected: %i", rndnum);
        switch(rndnum) {
        case 0:
          pktsz = sizeof(example_pkt1);
          pkt = example_pkt1;
          break;
        case 1:
          pktsz = sizeof(example_pkt2);
          pkt = example_pkt2;
          break;
        case 2:
          pktsz = sizeof(example_pkt3);
          pkt = example_pkt3;
          break;
        case 3:
          pktsz = sizeof(example_pkt4);
          pkt = example_pkt4;
          break;
        case 4:
          pktsz = sizeof(example_pkt5);
          pkt = example_pkt5;
          break;
        case 5:
          pktsz = sizeof(example_pkt6);
          pkt = example_pkt6;
          break; 
        default:
          logger(MSG_INFO, "Out of bounds, can't send this one, try again");
          return;    
      }
    if (write(usbfd, pkt, pktsz) < 0) {
      logger(MSG_ERROR, "%s: Error sending new CB message %i\n", __func__, rndnum);
    } else {
      logger(MSG_INFO, "%s: Sent CB message %i\n", __func__, rndnum);
    }
  } else if (message_id == 0) {
    pktsz = sizeof(example_pkt1);
    example_pkt1[29] = 0x11; // Set page to 1/1 instead of 1/6
    pkt = example_pkt1;
    if (write(usbfd, pkt, pktsz) < 0) {
      logger(MSG_ERROR, "%s: Error sending new CB message\n", __func__);
    } else {
      logger(MSG_INFO, "%s: Sent CB message\n", __func__);
    }
  }
  pkt = NULL;
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
  atcmd =
      (struct atcmd_reg_request *)calloc(1, sizeof(struct atcmd_reg_request));
  atcmd->ctlid = 0x00;
  atcmd->transaction_id = htole16(tid);
  atcmd->msgid = htole16(32);
  atcmd->dummy1 = 0x01;
  atcmd->dummy2 = 0x0100;
  atcmd->dummy3 = 0x00;
  atcmd->packet_size = strlen(command) + 40 +
                       sizeof(struct atcmd_reg_request); // atcmd + command + 36
  atcmd->command_length = strlen(command);
  atcmd->var2 = strlen(command) + 2; // It has to be something like this
  atcmd->var1 = atcmd->var2 + 3;

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
    syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
            LINUX_REBOOT_CMD_POWER_OFF, NULL);
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
    syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
            LINUX_REBOOT_CMD_RESTART2, "recovery");
    break;
  case 116: // Is custom? alwas answer yes
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 117: // Enable PCM16k
    response->result = 2;
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 118: // Enable PCM48K
    response->result = 2;
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 119: // Enable PCM8K (disable pcmhi)
    response->result = 2;
    sckret = send_pkt(qmidev, response, pkt_size);
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
    syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
            LINUX_REBOOT_CMD_RESTART, NULL);
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 124: // Custom alert tone ON
    sckret = send_pkt(qmidev, response, pkt_size);
    set_custom_alert_tone(true);       // save to flash
    break;
  case 125: // Custom alert tone off
    sckret = send_pkt(qmidev, response, pkt_size);
    set_custom_alert_tone(false);       // save to flash
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
    usleep(
        300); // Give it some time to be able to reach the ADSP before rebooting
    syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
            LINUX_REBOOT_CMD_RESTART2, "bootloader");
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
    fp = fopen(get_openqti_logfile(), "r");
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
    set_log_level(MSG_DEBUG);
    break;
  case 133: // Runtime set log level to info
    sckret = send_pkt(qmidev, response, pkt_size);
    set_log_level(MSG_INFO);
    break;
  case 134: // Simulate SMS
    bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",%i\r\n", get_internal_pending_messages() + get_pending_messages_in_adsp());
    response->replysz = htole16(bytes_in_reply); // Size of the string to reply
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    inject_message(0);
    pulse_ring_in();
    break;
  case 135: // Simulate SMS Notification
    bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",%i\r\n", get_internal_pending_messages() + get_pending_messages_in_adsp());
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
    set_automatic_call_recording(true);
    break;
  case 141:
    sckret = send_pkt(qmidev, response, pkt_size);
    set_automatic_call_recording(false);
    break;
  case 142: // Wipe message storage
    sckret = send_pkt(qmidev, response, pkt_size);
    wipe_message_storage();
    break;
  case 143: // Fake secure boot status response
    bytes_in_reply =
        sprintf(response->reply, "\r\n+QCFG: \"secbootstat\",0\r\n");
    response->replysz = htole16(bytes_in_reply); // Size of the string to reply
    pkt_size = sizeof(struct at_command_respnse) -
               (MAX_REPLY_SZ -
                bytes_in_reply); // total size - (max string - used string)
    sckret = send_pkt(qmidev, response, pkt_size);
    break;
  case 144: // Send example #1 CB message
      sckret = send_pkt(qmidev, response, pkt_size);
      atfwd_runtime_state.debug_cb = true;
      break;

  case 145: // Send a random example CB message from our list
      sckret = send_pkt(qmidev, response, pkt_size);
      atfwd_runtime_state.random_debug_cb = true;
      break;

  case 146: // Send a random example CB message from our list
      sckret = send_pkt(qmidev, response, pkt_size);
      atfwd_runtime_state.stream_cb = true;
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
  if (sckret < 0) {
    logger(MSG_ERROR, "%s: Send pkt failed!\n", __func__);
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

  bytes_in_reply = sprintf(response->reply, "\r\n+CMTI: \"ME\",%i\r\n", get_internal_pending_messages());
  response->replysz = htole16(bytes_in_reply);
  pkt_size =
      sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);
  sckret = send_pkt(qmidev, response, pkt_size);
  if (sckret < 0) {
    logger(MSG_ERROR, "%s: Send pkt failed!\n", __func__);
  }
  set_pending_notification_source(MSG_EXTERNAL);
  set_notif_pending(true);

  qmidev->transaction_id++;
  free(response);
  response = NULL;
  return 0;
}

/* Send URC asking for data */
int at_send_missing_cellid_data(struct qmi_device *qmidev) {
  int sckret;
  struct at_command_respnse *response;
  int pkt_size;
  int bytes_in_reply = 0;

  /* Build initial response data */
  response = calloc(1, sizeof(struct at_command_respnse));
  response->qmipkt.ctlid = 0x00;
  response->qmipkt.transaction_id = htole16(qmidev->transaction_id);
  qmidev->transaction_id++;
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

  bytes_in_reply = sprintf(response->reply, "\r\n+MISSINGOCID: %s-%s\r\n", (char*)get_current_mcc(), (char*)get_current_mnc());
  response->replysz = htole16(bytes_in_reply);
  pkt_size =
      sizeof(struct at_command_respnse) - (MAX_REPLY_SZ - bytes_in_reply);
  sckret = send_pkt(qmidev, response, pkt_size);
  if (sckret < 0) {
    logger(MSG_ERROR, "%s: Send pkt failed!\n", __func__);
  }

//  qmidev->transaction_id++;
  free(response);
  response = NULL;
  return 0;
}
/* Register AT commands into the DSP */
int init_atfwd(struct qmi_device *qmidev) {
  int j, ret;
  ssize_t pktsize;
  char *atcmd;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  socklen_t addrlen = sizeof(struct sockaddr_msm_ipc);
  char buf[MAX_PACKET_SIZE];
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
  int ret;
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
    select(MAX_FD, &readfds, NULL, NULL, &tv);
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

    if (nas_is_network_in_service() && is_cellid_data_missing() == 0) {
      logger(MSG_INFO, "%s: Fire the pending cell id notification!\n", __func__);
      at_send_missing_cellid_data(at_qmi_dev);
      get_opencellid_data();

    }
  }
  // Close AT socket
  close(at_qmi_dev->fd);
  free(at_qmi_dev);
}
