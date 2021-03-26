#include "../inc/openqti.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/ipc_security.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

/*
  OpenQTI reworked.
  Attempt 2
 */

uint8_t current_call_state;
bool debug_to_stdout;

struct mixer *mixer;
struct pcm *pcm_tx;
struct pcm *pcm_rx;

struct {
  int rmnet_ctrl;
  int dpm_ctrl;
  int smd_ctrl;
} node;

void logger(uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  if (debug_to_stdout) {
    fd = stdout;
  } else {
    fd = fopen("/var/log/openqti.log", "a");
    if (fd < 0) {
      logger(MSG_ERROR, "[%s] Error opening logfile \n", __func__);
      fd = stdout;
    }
  }
  switch (level) {
  case 0:
    fprintf(fd, "[DBG]");
    break;
  case 1:
    fprintf(fd, "[WRN]");
    break;
  default:
    fprintf(fd, "[ERR]");
    break;
  }
  va_start(args, format);
  vfprintf(fd, format, args);
  va_end(args);
  fflush(fd);
  if (fd != stdout) {
    fclose(fd);
  }
}

void dump_packet(char *direction, char *buf) {
  int i;
  logger(MSG_DEBUG, "%s :", direction);
  for (i = 0; i < sizeof(buf); i++) {
    logger(MSG_DEBUG, "0x%02x ", buf[i]);
  }
  logger(MSG_DEBUG, "\n");
}

int write_to(const char *path, const char *val, int flags) {
  int ret;
  int fd = open(path, flags);
  if (fd < 0) {
    return ENOENT;
  }
  ret = write(fd, val, strlen(val) * sizeof(char));
  close(fd);
  return ret;
}

/* Setup initial serivce permissions on the IPC router.
   You need to send it something before you can start
   using it. This sets it up so root and a user with
   UID 54 / GID 54 (modemusr in my custom yocto) can
   access it */
int setup_ipc_security() {
  int fd;
  int i;
  struct irsc_rule *cur_rule;
  int ret = 0;
  int ipc_categories = 511;
  logger(MSG_DEBUG, "[%s] Setting up MSM IPC Router security...\n", __func__);
  fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  if (!fd) {
    logger(MSG_ERROR, " Error opening socket \n");
    return -1;
  }
  for (i = 0; i < ipc_categories; i++) {
    cur_rule = calloc(1, (sizeof(*cur_rule) + sizeof(uint32_t)));
    cur_rule->rl_no = 54;
    cur_rule->service = i;
    cur_rule->instance = 4294967295; // all instances
    cur_rule->group_id[0] = 54;
    if (ioctl(fd, IOCTL_RULES, cur_rule) < 0) {
      logger(MSG_ERROR, "[%s] Error serring rule %i \n", __func__, i);
      ret = 2;
      break;
    }
  }

  close(fd);
  if (ret != 0) {
    logger(MSG_ERROR, "[%s] Error uploading rules (%i) \n", __func__, ret);
  } else {
    logger(MSG_DEBUG, "[%s] Upload finished. \n", __func__);
  }

  return ret;
}

/* This needs to go. It's a horrible way of finding there's a call
    I haven't found a way to actually do this right yet. Tried
    with ATFWD too but it doesn't get ATDT / ATA events*/
void handle_pkt_action(char *pkt, int from) {
  /* All packets passing between rmnet_ctl and smdcntl8 seem to be 4 byte long
   */
  /* This is messy and doesn't work correctly. Sometimes there's a flood and
     this gets hit leaving audio on for no reason, need to find a reliable way
     of knowing if there's an actual call */
  /* All calls report these codes from the DSP, usually preceeded by a 0x10.
          Problem is depending on modem status, the packets don't really arrive
          one after the other and you have some other stuff in between. Maybe
     keeping track of last 4 or 8 bytes is enough to track if it's a call or
     something else */
  if (sizeof(pkt) > 3 && from == FROM_DSP) {
    if (pkt[0] == 0x1) {
      switch (pkt[1]) {
      case 0x40:
        logger(MSG_DEBUG, "Outgoing Call start: VoLTE\n");
        start_audio(CALL_MODE_VOLTE);
        break;
      case 0x38:
        logger(MSG_DEBUG, "Outgoing Call start: CS Voice\n");
        start_audio(CALL_MODE_CS);
        break;
      case 0x46:
        logger(MSG_DEBUG, "Incoming Call start : CS Voice\n");
        start_audio(CALL_MODE_CS); // why did I set this to VoLTE previously?
        break;
      case 0x3c: // or 0x42
      case 0x42: // 0x4a too?
        logger(MSG_DEBUG, "Incoming Call start : VoLTE\n");
        start_audio(CALL_MODE_VOLTE);
        break;
      case 0x41:
      case 0x69: // Hangup from CSVoice
      case 0x5f: // Hangup from LTE?
        logger(MSG_DEBUG, "Hang up! \n");
        stop_audio();
        break;
      }
    }
  }
}

int set_mixer_ctl(struct mixer *mixer, char *name, int value) {
  struct mixer_ctl *ctl;
  ctl = get_ctl(mixer, name);
  int r;

  r = mixer_ctl_set_value(ctl, 1, value);
  if (r < 0) {
    logger(MSG_ERROR, "%s: Setting %s to value %i failed \n", __func__, name,
           value);
  }
  return 0;
}

int stop_audio() {
  if (current_call_state == 0) {
    logger(MSG_ERROR, "%s: No call in progress \n", __func__);
    return 1;
  }
  if (pcm_tx == NULL || pcm_rx == NULL) {
    logger(MSG_ERROR, "%s: Invalid PCM, did it fail to open?\n",__func__);
  }
  if (pcm_tx->fd >= 0)
    pcm_close(pcm_tx);
  if (pcm_rx->fd >= 0)
    pcm_close(pcm_rx);

  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "error opening mixer! %s: %d\n", strerror(errno),
           __LINE__);
    return 0;
  }

  // We close all the mixers
  if (current_call_state == 1) {
    set_mixer_ctl(mixer, TXCTL_VOICE, 0); // Playback
    set_mixer_ctl(mixer, RXCTL_VOICE, 0); // Capture
  } else if (current_call_state == 2) {
    set_mixer_ctl(mixer, TXCTL_VOLTE, 0); // Playback
    set_mixer_ctl(mixer, RXCTL_VOLTE, 0); // Capture
  }

  mixer_close(mixer);
  current_call_state = 0;
  return 1;
}

/*	Setup mixers and open PCM devs
 *		type: 0: CS Voice Call
 *		      1: VoLTE Call
 * If a call wasn't actually in progress the kernel
 * will complain with ADSP_FAILED / EADSP_BUSY
 */
int start_audio(int type) {
  int i;
  char pcm_device[18];
  if (current_call_state > 0) {
    logger(MSG_ERROR, "%s: Audio already active, restarting... \n", __func__);
    stop_audio();
  }

  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "%s: Error opening mixer!\n", __func__);
    return 0;
  }

  switch (type) {
  case 1:
    logger(MSG_DEBUG, "Call in progress: Circuit Switch\n");
    set_mixer_ctl(mixer, TXCTL_VOICE, 1); // Playback
    set_mixer_ctl(mixer, RXCTL_VOICE, 1); // Capture
    strncpy(pcm_device, PCM_DEV_VOCS, sizeof(PCM_DEV_VOCS));
    break;
  case 2:
    logger(MSG_DEBUG, "Call in progress: VoLTE\n");
    set_mixer_ctl(mixer, TXCTL_VOLTE, 1); // Playback
    set_mixer_ctl(mixer, RXCTL_VOLTE, 1); // Capture
    strncpy(pcm_device, PCM_DEV_VOLTE, sizeof(PCM_DEV_VOLTE));
    break;
  default:
    logger(MSG_ERROR, "%s: Can't set mixers, unknown call type %i\n", __func__,
           type);
    return -EINVAL;
  }
  mixer_close(mixer);

  pcm_rx = pcm_open((PCM_IN | PCM_MONO), pcm_device);
  pcm_rx->channels = 1;
  pcm_rx->rate = 8000;
  pcm_rx->flags = PCM_IN | PCM_MONO;

  pcm_tx = pcm_open((PCM_OUT | PCM_MONO), pcm_device);
  pcm_tx->channels = 1;
  pcm_tx->rate = 8000;
  pcm_tx->flags = PCM_OUT | PCM_MONO;

  if (set_params(pcm_rx, PCM_IN)) {
    logger(MSG_ERROR, "Error setting RX Params\n");
    pcm_close(pcm_rx);
    return -EINVAL;
  }

  if (set_params(pcm_tx, PCM_OUT)) {
    logger(MSG_ERROR, "Error setting TX Params\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
    logger(MSG_ERROR, "Error getting RX PCM ready\n");
    pcm_close(pcm_rx);
    return -EINVAL;
  }

  if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
    logger(MSG_ERROR, "Error getting TX PCM ready\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_START) < 0) {
    logger(MSG_ERROR, "PCM ioctl start failed for TX\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_START) < 0) {
    logger(MSG_ERROR, "PCM ioctl start failed for RX\n");
    pcm_close(pcm_rx);
  }

  if (type == 1) {
    current_call_state = 1;
  } else if (type == 2) {
    current_call_state = 2;
  }
  return 0;
}

int dump_audio_mixer() {
  struct mixer *mixer;
  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "%s: Error opening mixer!\n", __func__);
    return -1;
  }
  mixer_dump(mixer);
  mixer_close(mixer);
  return 0;
}

/* Connect to DPM and request opening SMD Control port 8 */
int init_port_mapper(struct qmi_device *qmidev) {
  int ret, linestate;
  struct timeval tv;
  struct portmapper_open_request dpmreq;
  char buf[MAX_PACKET_SIZE];
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  socklen_t addrlen = sizeof(struct sockaddr_msm_ipc);

  ret = open_ipc_socket(qmidev, IPC_HEXAGON_NODE, IPC_HEXAGON_DPM_PORT, 0x2f,
                        0x1, IPC_ROUTER_DPM_ADDRTYPE); // open DPM service
  if (ret < 0) {
    logger(MSG_ERROR, "[%s] Error opening IPC Socket!\n", __func__);
    return -EINVAL;
  }

  if (ioctl(qmidev->fd, IOCTL_BIND_TOIPC, 0) < 0) {
    logger(MSG_ERROR, "IOCTL to the IPC1 socket failed \n");
  }

  node.rmnet_ctrl = open(RMNET_CTL, O_RDWR);
  if (node.rmnet_ctrl < 0) {
    logger(MSG_ERROR, "Error opening %s \n", RMNET_CTL);
    return -EINVAL;
  }

  node.dpm_ctrl = open(DPM_CTL, O_RDWR);
  if (node.dpm_ctrl < 0) {
    logger(MSG_ERROR, "Error opening %s \n", DPM_CTL);
    return -EINVAL;
  }
  // Unknown IOCTL, just before line state to rmnet
  if (ioctl(node.dpm_ctrl, _IOC(_IOC_READ, 0x72, 0x2, 0x4), &ret) < 0) {
    logger(MSG_ERROR, "IOCTL failed: %i\n", ret);
  }

  ret = setsockopt(qmidev->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (ret) {
    logger(MSG_ERROR, "Error setting socket options \n");
  }

  /* Create the SMD Open request packet */
  /* Parts of this are probably _very_ wrong*/
  /* Recreate the magic packet that request the port mapper to open smdcntl8 */
  dpmreq.ctlid = 0x00;
  dpmreq.transaction_id = htole16(1);
  dpmreq.msgid = htole16(32);
  dpmreq.length = sizeof(struct portmapper_open_request) -
                  (3 * sizeof(uint16_t)) -
                  sizeof(uint8_t); // Size of the packet minux qmi header
  dpmreq.is_valid_ctl_list = htole16(0x10);
  dpmreq.ctl_list_length =
      0x0b010015; // sizeof(SMDCTLPORTNAME); this has to be wrong...

  strncpy(dpmreq.hw_port_map[0].port_name, SMDCTLPORTNAME,
          sizeof(SMDCTLPORTNAME));
  dpmreq.hw_port_map[0].epinfo.ep_type = htole16(DATA_EP_TYPE_BAM_DMUX);
  dpmreq.hw_port_map[0].epinfo.peripheral_iface_id = 0x08000000;

  dpmreq.is_valid_hw_list = 0x00;
  dpmreq.hw_list_length = 0x11110000;
  dpmreq.hw_epinfo.ph_ep_info.ep_type = DATA_EP_TYPE_RESERVED;
  dpmreq.hw_epinfo.ph_ep_info.peripheral_iface_id = 0x00000501;

  dpmreq.hw_epinfo.ipa_ep_pair.cons_pipe_num =
      0x00000800; // 6 is what the bam responds but packet seems to say 0
  dpmreq.hw_epinfo.ipa_ep_pair.prod_pipe_num = htole32(0);

  dpmreq.is_valid_sw_list = false;
  dpmreq.sw_list_length = 0;

  do {
    sleep(1);
    logger(MSG_ERROR,
           "[%s] Waiting for the Dynamic port mapper to become ready... \n",
           __func__);
  } while (sendto(qmidev->fd, &dpmreq, sizeof(dpmreq), MSG_DONTWAIT,
                  (void *)&qmidev->socket, sizeof(qmidev->socket)) < 0);
  logger(MSG_DEBUG, "[%s] DPM Request completed!\n", __func__);
  do {
    ret = recvfrom(qmidev->fd, buf, sizeof(buf), 0,
                   (struct sockaddr *)&qmidev->socket, &addrlen);

    // Wait one second after requesting bam init...
    node.smd_ctrl = open(SMD_CNTL, O_RDWR);
    if (node.smd_ctrl < 0) {
      logger(MSG_ERROR, "Error opening %s, retry... \n", SMD_CNTL);
    }
    // Sleep 1 second just in case it needs to loop
    /* The modem needs some more time to boot the DSP
     * Since I haven't found a way to query its status
     * except by probing it, I loop until the IPC socket
     * manages to open /dev/smdcntl8, which is the point
     * where it is ready
     */
    sleep(1);
  } while (node.smd_ctrl < 0);

  close(node.dpm_ctrl); // We won't need DPM anymore
  // All the rest is moved away from here and into the init
  return 0;
}

char *make_packet(int tid, const char *command) {
  struct atcmd_reg_request *atcmd;
  char *buf;
  int k, i;
  ssize_t cmdsize;
  ssize_t bufsize;
  atcmd =
      (struct atcmd_reg_request *)calloc(1, sizeof(struct atcmd_reg_request));
  // memset (atcmd,0xbb,sizeof(struct atcmd_reg_request));
  // atcmd->command = calloc(sizeof(char), 1); //WTF DOES IT NEED 43 padding
  // zeros!
  // memset (atcmd->command,0x00,sizeof(atcmd->command));
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
  // strncpy(atcmd->command, command, strlen(command));

  /*
  VAR3 renamed to strlength, as it's always the length of the string being
  passed +1 but removing the "+" sign ($QCPWRDN...) VAR2: When string is 4
  chars, it's always 0x07, When string is 8 chars, it's always 0x0a When string
  is 3 chars, it's always 0x06 VAR1 is always 3 more than VAR2
  */
  atcmd->var2 = strlen(command) + 2; // It has to be something like this
  atcmd->var1 = atcmd->var2 + 3;
  bufsize = sizeof(struct atcmd_reg_request) +
            ((strlen(command) + 43) * sizeof(char));
  buf = calloc(256, sizeof(uint8_t));
  memcpy(buf, atcmd, sizeof(struct atcmd_reg_request));
  memcpy(buf + sizeof(struct atcmd_reg_request), command,
         (sizeof(char) * strlen(command)));
  return buf;
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
  socklen_t addrlen = sizeof(struct sockaddr_msm_ipc);
  char buf[MAX_PACKET_SIZE];
  uint32_t node_id, port_id;
  struct msm_ipc_port_addr at_port;
  at_port = get_node_port(2, 29, 1);

  printf("Node: 0x%.2x, Port: 0x%.2x\n", at_port.node_id, at_port.port_id);
  logger(MSG_DEBUG, "[%s] Connecting to IPC... \n", __func__);
  ret = open_ipc_socket(qmidev, at_port.node_id, at_port.port_id,
                        at_port.node_id, at_port.port_id,
                        IPC_ROUTER_AT_ADDRTYPE); // open ATFWD service
  if (ret < 0) {
    logger(MSG_ERROR, "[%s] Error opening socket \n", __func__);
    return -EINVAL;
  }

  ret = setsockopt(qmidev->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (ret) {
    logger(MSG_ERROR, "Error setting socket options \n");
  }

  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    logger(MSG_DEBUG, "[%s] --> CMD %i: %s ", __func__,
           at_commands[j].command_id, at_commands[j].cmd);
    atcmd = make_packet(qmidev->transaction_id, at_commands[j].cmd);
    qmidev->transaction_id++;

    pktsize = sizeof(struct atcmd_reg_request) +
              ((strlen(at_commands[j].cmd) + 47) * sizeof(char));
    ret = sendto(qmidev->fd, atcmd, pktsize, MSG_DONTWAIT,
                 (void *)&qmidev->socket, sizeof(qmidev->socket));
    if (ret < 0) {
      logger(MSG_DEBUG, "Failed (%i) \n", ret);
    }
    ret = recvfrom(qmidev->fd, buf, sizeof(buf), 0,
                   (struct sockaddr *)&qmidev->socket, &addrlen);
    // ret = read(qmidev->fd, &buf, MAX_PACKET_SIZE);
    if (ret < 0) {
      logger(MSG_DEBUG, "Sent but rejected (%i) \n", ret);
    }
    if (ret == 14) {
      for (i = 0; i < ret; i++) {
        printf("0x%.2x ", buf[i]);
      }
      printf("\n");
      if (buf[12] == 0x00) {
        logger(MSG_DEBUG, "Command accepted (0x00) \n", ret);
      } else if (buf[12] == 0x01) {
        logger(MSG_DEBUG, "Sent but rejected (0x01) \n", ret);
      }
      /* atfwd_daemon responds 0x30 on success, but 0x00 will do too.
        When it responds 0x02 or 0x01 byte 12 it always fail, no fucking clue
        why */
    } else {
      printf("UNKNOWN RESPONSE\n");
      for (i = 0; i < ret; i++) {
        printf("0x%.2x ", buf[i]);
      }
      printf("\n");
    }
  }

  return 0;
}

/* When a command is requested via AT interface,
   this function is used to answer to them */
int handle_atfwd_response(struct qmi_device *qmidev, char *buf,
                          unsigned int sz) {
  int j, sckret;
  int packet_size;
  uint8_t cmdsize, cmdend;
  char *parsedcmd;
  int cmd_id = -1;
  struct at_command_simple_reply *cmdreply;
  cmdreply = calloc(1, sizeof(struct at_command_simple_reply));
  char dummyok[] = {0x00, 0x00, 0x00, 0x22, 0x00, 0x0b, 0x00, 0x01, 0x08,
                    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00};
  logger(MSG_DEBUG, "Handle response: not implemented \n");
  if (sz < 16) {
    logger(MSG_ERROR, "Packet too small\n");
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
  strncpy(parsedcmd, buf + 19, cmdsize);

  logger(MSG_DEBUG, "Command is %s\n", parsedcmd);
  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    if (strcmp(parsedcmd, at_commands[j].cmd) == 0) {
      logger(MSG_DEBUG, "Command match: %s, ID %i\n", at_commands[j].cmd,
             at_commands[j].command_id);
      cmd_id = at_commands[j].command_id;
    }
  }

  dummyok[1] = htole16(qmidev->transaction_id);
  cmdreply->ctlid = 0x04;
  cmdreply->transaction_id = htole16(qmidev->transaction_id);
  cmdreply->msgid = htole16(33);
  cmdreply->length = sizeof(struct at_command_simple_reply) -
                     (3 * sizeof(uint16_t)) - sizeof(uint8_t);
  cmdreply->command = 0;
  cmdreply->result = 1;
  cmdreply->response = 3;
  /*
  Not expecting this to work, just finding a way of not needing
  to reboot everytime I send a command for now. I'll worry about
  correctness later */
  // lets try to just bitflip the response type
  buf[12] = 0x01;
  if (cmd_id == -1) { // Send an error
    logger(MSG_ERROR, "Puking error as success :)\n");
    sckret = sendto(qmidev->fd, dummyok, sizeof(dummyok), MSG_DONTWAIT,
                    (void *)&qmidev->socket, sizeof(qmidev->socket));

  } else {
    logger(MSG_ERROR, "Puking success as success\n");
    sckret = sendto(qmidev->fd, dummyok, sizeof(dummyok), MSG_DONTWAIT,
                    (void *)&qmidev->socket, sizeof(qmidev->socket));

    switch (cmd_id) {
    case 72: // fastboot
      system("reboot");
      break;

    default: // Not implemented, send an OK to everything for now
      break;
    }
  }
  logger(MSG_DEBUG, "socket send res: %i\n", sckret);
  qmidev->transaction_id++;
  return 0;
}

int main(int argc, char **argv) {
  int i, ret, pret;
  bool debug = false;
  int max_fd = 20;
  int linestate;
  fd_set readfds;
  char buf[MAX_PACKET_SIZE];
  struct qmi_device *portmapper_qmi_dev;
  struct qmi_device *at_qmi_dev;
  portmapper_qmi_dev = calloc(1, sizeof(struct qmi_device));
  at_qmi_dev = calloc(1, sizeof(struct qmi_device));
  logger(MSG_DEBUG, "Welcome to OpenQTI \n", __func__);

  while ((ret = getopt(argc, argv, "adbu")) != -1)
    switch (ret) {
    case 'a':
      fprintf(stdout, "Dump audio mixer data \n");
      return dump_audio_mixer();
      break;
    case 'd':
      fprintf(stdout, "Debug mode\n");
      debug_to_stdout = true;
      break;
    case 'u':
      printf("Lookup services: Address type? 2\n");
      find_services(2);

      return 0;
      break;
      /*	case 'u':
                      fprintf(stdout,"Restart USB on start\n");
                      reset_usb = true;
                      bypass_mode = false;
                      break;*/
    case '?':
      fprintf(stdout, "Options:\n");
      fprintf(stdout, " -a: Dump audio mixer controls\n");
      fprintf(stdout, " -d: Send logs to stdout\n");
      return 1;
    default:
      debug_to_stdout = false;
      break;
    }

  if (write_to(CPUFREQ_PATH, CPUFREQ_PERF, O_WRONLY) < 0) {
    logger(MSG_ERROR, "[%s] Error setting up governor in performance mode\n",
           __func__);
  }

  do {
    logger(MSG_DEBUG, "[%s] Waiting for ADSP init...\n", __func__);
  } while (!is_server_active(IPC_HEXAGON_NODE, IPC_HEXAGON_DPM_PORT));

  logger(MSG_DEBUG, "[%s] Init: IPC Security settings\n", __func__);
  if (setup_ipc_security() != 0) {
    logger(MSG_ERROR, "[%s] Error setting up MSM IPC Security!\n", __func__);
  }

  logger(MSG_DEBUG, "[%s] Init: Dynamic Port Mapper \n", __func__);
  if (init_port_mapper(portmapper_qmi_dev) < 0) {
    logger(MSG_ERROR, "[%s] Error setting up port mapper!\n", __func__);
  }

  logger(MSG_DEBUG, "[%s] Init: AT Command forwarder \n", __func__);
  if (init_atfwd(at_qmi_dev) < 0) {
    logger(MSG_ERROR, "[%s] Error setting up ATFWD!\n", __func__);
  }

  /* QTI gets line state, then sets modem offline and online while initializing
   */
  ret = ioctl(node.rmnet_ctrl, GET_LINE_STATE, &linestate);
  if (ret < 0)
    logger(MSG_ERROR, "Get line  RMNET state: %i, %i \n", linestate, ret);

  printf("Linestate is %i, round \n", linestate);

  // Set modem OFFLINE AND ONLINE
  ret = ioctl(node.rmnet_ctrl, MODEM_OFFLINE);
  if (ret < 0)
    logger(MSG_ERROR, "Set modem offline: %i \n", ret);

  ret = ioctl(node.rmnet_ctrl, MODEM_ONLINE);
  if (ret < 0)
    logger(MSG_ERROR, "Set modem online: %i \n", ret);

  // close(qmidev->fd); // We can get rid of this socket too
  logger(MSG_DEBUG, "[%s] Init: Setup default I2S Audio settings \n", __func__);
  for (i = 0; i < (sizeof(sysfs_value_pairs) / sizeof(sysfs_value_pairs[0]));
       i++) {
    if (write_to(sysfs_value_pairs[i].path, sysfs_value_pairs[i].value,
                 O_RDWR) < 0) {
      logger(MSG_ERROR, "[%s] Error writing to %s\n", __func__,
             sysfs_value_pairs[i].path);
    } else {
      logger(MSG_DEBUG, "[%s]  --> Written %s to %s \n", __func__,
             sysfs_value_pairs[i].value, sysfs_value_pairs[i].path);
    }
  }
  logger(MSG_DEBUG, "[%s] Modem ready!\n", __func__);

  if (write_to(CPUFREQ_PATH, CPUFREQ_PS, O_WRONLY) < 0) {
    logger(MSG_ERROR, "[%s] Error setting up governor in powersave mode\n",
           __func__);
  }
  /* This pipes messages between rmnet_ctl and smdcntl8,
     and reads the IPC socket in case there's a pending
     AT command to answer to */
  printf(" Modem ready, looping... \n");
  while (1) {
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(node.rmnet_ctrl, &readfds);
    FD_SET(node.smd_ctrl, &readfds);
    FD_SET(at_qmi_dev->fd, &readfds);
    pret = select(max_fd, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(node.rmnet_ctrl, &readfds)) {
      ret = read(node.rmnet_ctrl, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        //	handle_pkt_action(buf, FROM_HOST);
        ret = write(node.smd_ctrl, buf, ret);
        dump_packet("rmnet_ctrl --> smdcntl8", buf);
      }
    } else if (FD_ISSET(node.smd_ctrl, &readfds)) {
      ret = read(node.smd_ctrl, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        //	 handle_pkt_action(buf, FROM_DSP);
        ret = write(node.rmnet_ctrl, buf, ret);
        dump_packet("smdcntl8 --> rmnet_ctrl", buf);
      }
    } else if (FD_ISSET(at_qmi_dev->fd, &readfds)) {
      printf("-->ATFWD PACKET\n");
      ret = read(at_qmi_dev->fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        handle_atfwd_response(at_qmi_dev, buf, ret);
      }
    }
  }
}