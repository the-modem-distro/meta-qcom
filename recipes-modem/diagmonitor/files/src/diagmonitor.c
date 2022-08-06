// SPDX-License-Identifier: MIT

#include "diagmonitor.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

/*
 *  Diag Monitor
 *  Based on GSM-Parser https://github.com/E3V3A/gsm-parser
 *           Snoop Snitch https://github.com/E3V3A/snoopsnitch
 *           Mobile Insight
 * https://github.com/mobile-insight/mobileinsight-mobile/ And probably a lot
 * more... Yes, this is a mess Yes, I will clean it up at some point No, this
 * doesn't work yet
 */

// diag_logging_mode_param_t
const int mode_param[] = {MEMORY_DEVICE_MODE, -1, 0};
struct {
  bool run;
} diag_runtime;

int use_mdm = 0;

void printpkt(uint8_t *buffer, size_t len) {
  int count = 0;
  int addr = 0;
  fprintf(stdout, "00000000 ");
  for (int i = 0; i < len; i++) {
    fprintf(stdout, "%.2x ", buffer[i]);
    if (count > 15) {

      fprintf(stdout, " |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c|\n%.8x ",
              buffer[i - 15] == 0x00 ? '.' : buffer[i - 15],
              buffer[i - 14] == 0x00 ? '.' : buffer[i - 14],
              buffer[i - 13] == 0x00 ? '.' : buffer[i - 13],
              buffer[i - 12] == 0x00 ? '.' : buffer[i - 12],
              buffer[i - 11] == 0x00 ? '.' : buffer[i - 11],
              buffer[i - 10] == 0x00 ? '.' : buffer[i - 10],
              buffer[i - 9] == 0x00 ? '.' : buffer[i - 9],
              buffer[i - 8] == 0x00 ? '.' : buffer[i - 8],
              buffer[i - 7] == 0x00 ? '.' : buffer[i - 7],
              buffer[i - 6] == 0x00 ? '.' : buffer[i - 6],
              buffer[i - 5] == 0x00 ? '.' : buffer[i - 5],
              buffer[i - 4] == 0x00 ? '.' : buffer[i - 4],
              buffer[i - 3] == 0x00 ? '.' : buffer[i - 3],
              buffer[i - 2] == 0x00 ? '.' : buffer[i - 2],
              buffer[i - 1] == 0x00 ? '.' : buffer[i - 1],
              buffer[i] == 0x00 ? '.' : buffer[i], addr + 0xF);
      count = 0;
      addr += 0xF;
    } else {
      count++;
    }
  }
  fprintf(stdout, " |");
  for (int k = 0; k < count; k++) {
    fprintf(stdout, "%c",
            buffer[len - count + k] == 0x00 ? '.' : buffer[len - count + k]);
  }
  fprintf(stdout, " |\n");
  fprintf(stdout, "As string: %s\n", buffer);
}

int write_config(int diagfd) {
  fprintf(stdout, "%s: Begin\n", __func__);
  uint8_t command_queue_size = (sizeof(cmd_array) / sizeof(cmd_array[0]));
  uint8_t *sendbuf, *command_tmp;
  uint8_t buffer[MAX_BUF_SIZE];
  size_t offset, this_command_len, full_msg_len;

  for (int i = 0; i < command_queue_size; i++) {
    uint8_t needs_escaping = false;
    fprintf(stdout, " -> Processing command %i\n", i);
    // We dont want to overwrite userspace data type, and
    // we dont need the ID (otherwise it would be 8)
    offset = use_mdm ? 8 : 4;

    this_command_len = cmd_array[i].len;
    full_msg_len = this_command_len + offset + 3;
    sendbuf = calloc(full_msg_len, sizeof(uint8_t));
    command_tmp = calloc(full_msg_len, sizeof(uint8_t));

    *((int *)sendbuf) = htole32(USER_SPACE_DATA_TYPE);
    if (use_mdm)
      *((int *)sendbuf + 1) = -MDM;

    fprintf(stdout, "  -> Command %i: %ld byte (offset %ld bytes) \n", i,
            full_msg_len, offset);
    // Copy this blob
    memcpy(sendbuf + offset, cmd_array[i].cmd, this_command_len);
    memcpy(command_tmp, cmd_array[i].cmd, this_command_len);
    uint16_t crc;
    if (command_tmp[0] == 0x7D || command_tmp[0] == 0x7E) {
      needs_escaping = true;
      printf("ESCAPE! %ld --> ", this_command_len);
      command_tmp[1] = (cmd_array[i].cmd[1] ^ 0x20);
      this_command_len--;
      command_tmp++;
      printf(" %ld \n ", this_command_len);
    }
    crc = crc16(command_tmp, this_command_len);

    for (int k = 0; k < this_command_len; k++)
      fprintf(stdout, "%.2x ", command_tmp[k]);
    fprintf(stdout, "\n");
    /* two bytes CRC */
    sendbuf[full_msg_len - 3] = (uint8_t)(crc & 0xFF);
    sendbuf[full_msg_len - 2] = (uint8_t)(crc >> 8);
    sendbuf[full_msg_len - 1] = PACKET_START_STOP;

    for (int k = 0; k < full_msg_len; k++)
      fprintf(stdout, "%.2x ", sendbuf[k]);
    fprintf(stdout, "\n");
    char buf[256];
    snprintf(buf, 256, "echo 'Sending command %i' > /dev/kmsg\n", i);
    system(buf);
    //  sleep(1);
    int ret = write(diagfd, sendbuf, full_msg_len); // uint32_t + 4*uint8_t
    if (ret < 0) {
      fprintf(stderr, "  -> Command %i failed with ret code %i\n", i, ret);
      free(sendbuf);
      // We restore the original pointer address before freeing
      if (needs_escaping) {
        command_tmp--;
      }
      free(command_tmp);
      return -EINVAL;
    } else {
      fprintf(stderr, "Write result was %i\n", ret);
    }
    int rlen = read(diagfd, buffer, MAX_BUF_SIZE);
    if (rlen >= 0) {
      fprintf(stdout, "Command %i sent, Response was %i\n", i, rlen);
      // printpkt(buffer, rlen);
    } else {
      fprintf(stderr, "Error reading response to command %i\n", i);
      free(sendbuf);
      // We restore the original pointer address before freeing
      if (needs_escaping) {
        command_tmp--;
      }
      free(command_tmp);
      return -EINVAL;
    }

    free(sendbuf);
    // We restore the original pointer address before freeing
    if (needs_escaping) {
      command_tmp--;
    }
    free(command_tmp);
  }

  return 0;
}

int write_config2(int diagfd) {
  fprintf(stdout, "%s: BURN THE THING DOWN!\n", __func__);

  for (uint16_t i = 0; i < 0x2888; i++) {
  uint8_t *sendbuf, *command_tmp;
  uint8_t buffer[MAX_BUF_SIZE];
  size_t offset, this_command_len, full_msg_len;
    uint8_t needs_escaping = false;
    struct cmd_ext_message_config *config;
    config = calloc(1, sizeof(struct cmd_ext_message_config));
    fprintf(stdout, " -> Processing command %i\n", i);
    // We dont want to overwrite userspace data type, and
    // we dont need the ID (otherwise it would be 8)
    offset = use_mdm ? 8 : 4;

    config->id = 0x7d;
    config->sub_cmd = 0x04;
    config->ssid_in = i;
    config->ssid_out = i;
    config->padding = 0x00;
    config->mask = 0xffffffff;

    this_command_len = sizeof(struct cmd_ext_message_config);
    full_msg_len = this_command_len + offset + 3;
    sendbuf = calloc(full_msg_len, sizeof(uint8_t));
    command_tmp = calloc(full_msg_len, sizeof(uint8_t));

    *((int *)sendbuf) = htole32(USER_SPACE_DATA_TYPE);
    if (use_mdm)
      *((int *)sendbuf + 1) = -MDM;

    fprintf(stdout, "  -> Command %i: %ld byte (offset %ld bytes) \n", i,
            full_msg_len, offset);
    // Copy this blob

    fprintf(stdout, "memcpy\n");
    memcpy(sendbuf + offset, config, this_command_len);
    memcpy(command_tmp, config, this_command_len);
    uint16_t crc;
    if (command_tmp[0] == 0x7D || command_tmp[0] == 0x7E) {
      needs_escaping = true;
      printf("ESCAPE! %ld --> ", this_command_len);
      command_tmp[1] = (config->sub_cmd ^ 0x20);
      this_command_len--;
      command_tmp++;
      printf(" %ld \n ", this_command_len);
    }
    crc = crc16(command_tmp, this_command_len);

    /* two bytes CRC */
    sendbuf[full_msg_len - 3] = (uint8_t)(crc & 0xFF);
    sendbuf[full_msg_len - 2] = (uint8_t)(crc >> 8);
    sendbuf[full_msg_len - 1] = PACKET_START_STOP;

    for (int k = 0; k < full_msg_len; k++)
      fprintf(stdout, "%.2x ", sendbuf[k]);

    int ret = write(diagfd, sendbuf, full_msg_len); // uint32_t + 4*uint8_t
    if (ret < 0) {
      fprintf(stderr, "  -> Command %i failed with ret code %i\n", i, ret);
      free(sendbuf);
      free(config);
      if (needs_escaping) {
        command_tmp--;
      }
      free(command_tmp);
      return -EINVAL;
    } else {
      fprintf(stderr, "Write result was %i\n", ret);
    }
    int rlen = read(diagfd, buffer, MAX_BUF_SIZE);
    if (rlen >= 0) {
      fprintf(stdout, "Command %i sent, Response was %i\n", i, rlen);
      // printpkt(buffer, rlen);
    } else {
      fprintf(stderr, "Error reading response to command %i\n", i);
      free(sendbuf);
      free(config);
      if (needs_escaping) {
        command_tmp--;
      }
      free(command_tmp);
      return -EINVAL;
    }


    free(sendbuf);
    // We restore the original pointer address before freeing
    if (needs_escaping) {
      command_tmp--;
    }
    free(command_tmp);
    free(config);
  }

  return 0;
}

int client_id;
int setup_logging(int diagfd) {
  int ret;
  /* Find if we need to specify the remote processor */
  if (ioctl(diagfd, DIAG_IOCTL_REMOTE_DEV, &use_mdm) < 0) {
    fprintf(stderr, "DIAG_IOCTL_REMOTE_DEV failed :(\n");
  } else {
    fprintf(stdout, "Use MDM? %i\n", use_mdm);
  }

  // Register a DCI client
  struct diag_dci_reg_tbl_t dci_client;
  dci_client.client_id = 0;
  dci_client.notification_list = 0;
  dci_client.signal_type = SIGPIPE;
  dci_client.token = use_mdm;
  dci_client.token = 0;
  ret = ioctl(diagfd, DIAG_IOCTL_DCI_REG, &dci_client);
  if (ret < 0) {
    printf("DIAG_IOCTL_DCI_REG Failed :( %d\n", ret);
    return -EINVAL;
  } else {
    client_id = ret;
    printf("DIAG_IOCTL_DCI_REG Registered client %d :)\n", client_id);
  }

  /*
   * EXPERIMENTAL (NEXUS 6 ONLY): configure the buffering mode to circular
   */
  struct diag_buffering_mode_t buffering_mode;
  // buffering_mode.peripheral = remote_dev;
  buffering_mode.peripheral = 0;
  buffering_mode.mode = DIAG_BUFFERING_MODE_STREAMING;
  buffering_mode.high_wm_val = DEFAULT_HIGH_WM_VAL;
  buffering_mode.low_wm_val = DEFAULT_LOW_WM_VAL;

  ret = ioctl(diagfd, DIAG_IOCTL_PERIPHERAL_BUF_CONFIG, &buffering_mode);
  if (ret < 0) {
    printf("DIAG_IOCTL_PERIPHERAL_BUF_CONFIG failed :( %d\n", ret);
    return -EINVAL;
  } else {
    printf("DIAG_IOCTL_PERIPHERAL_BUF_CONFIG ok! :) %d\n", ret);
  }

  struct diag_logging_mode_param_t new_mode;
  new_mode.req_mode = MEMORY_DEVICE_MODE;
  new_mode.peripheral_mask = DIAG_CON_ALL;
  new_mode.mode_param = 0;
  ret = ioctl(diagfd, DIAG_IOCTL_SWITCH_LOGGING, &new_mode);

  if (ret < 0) {
    fprintf(stderr, "DIAG_IOCTL_SWITCH_LOGGING failed! :( %d\n", ret);
    return -EINVAL;
  } else {
    fprintf(stderr, "DIAG_IOCTL_SWITCH_LOGGING ok! :) %d\n", ret);
  }
  return 0;
}

void start_diag_thread() {
  int diagfd = -1;
  int pret, ret;
  fd_set readfds;
  uint8_t buffer[MAX_BUF_SIZE];
  fprintf(stdout, "%s: * START *\n", __func__);
  /* Step one is getting the diag interface ready */
  diagfd = open(DIAG_INTERFACE, O_RDWR | O_NONBLOCK);
  if (!diagfd) {
    fprintf(stderr, "%s: Error opening diag interface!\n", __func__);
    return;
  }

  if (setup_logging(diagfd) != 0) {
    fprintf(stderr, "Error setting up logging \n");
  }

  /* Second step will be the hardest one:
   * Tell the diag interface what we want to log so it gives us our data
   *
   */
  if (write_config(diagfd) != 0) {
    fprintf(stderr, "Error writing DIAG configuration params\n");
  }
/*
  if (write_config2(diagfd) != 0) {
    fprintf(stderr, "Error writing DIAG configuration params\n");
  }
*/
  /* Now we loop */
  uint32_t loopcnt = 0;
  struct cmd_log_message *message;
  while (diag_runtime.run) {
    printf("---->>>>>  Loop count %i\n", loopcnt);
    FD_ZERO(&readfds);
    memset(buffer, 0, MAX_BUF_SIZE);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_SET(diagfd, &readfds);

    pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(diagfd, &readfds)) {
      fprintf(stdout, "%s: Is Set! (loop %i)\n", __func__, loopcnt);
      ret = read(diagfd, &buffer, MAX_BUF_SIZE);
      fprintf(stdout, "PKT size is %i\n", ret);
      if (ret > 0 && ret >= sizeof(struct cmd_log_message)) {
        message = (struct cmd_log_message *)buffer;
        if (message->cmd == 0x79) {
          char tempbf[MAX_BUF_SIZE];

          if (message->num_args > 0 && message->num_args < 255) {
            memcpy(tempbf, message->args, message->num_args);
          } else {
            snprintf(tempbf, MAX_BUF_SIZE, "Error retrieving string");
          }
          fprintf(stdout,
                  "Incoming message (%.2x): Type %.2x, Argument size: %.2x, "
                  "Timestamp: %ld, Subsystem %d, line %d: %s\n",
                  message->cmd, message->type, message->num_args,
                  message->timestamp, message->subsystem_id, message->line,
                  tempbf);
          message = NULL;
        } else {
          printpkt(buffer, ret);
        }
      } else {
        fprintf(stdout, "Empty frame\n");
      }
    } else {
      fprintf(stdout, "Waiting... \n");
    }
    loopcnt++;
  }

  close(diagfd);
  fprintf(stdout, "Finish.\n");
}

int main(int argc, char **argv) {
  diag_runtime.run = true;
  start_diag_thread();
  return 0;
}
