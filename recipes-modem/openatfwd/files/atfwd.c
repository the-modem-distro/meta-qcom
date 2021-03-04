#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "atfwd.h"

/* ATFwd daemon replacement, non functional */
/* Shamelessly copying stuff from openqti as a start */
/* Based on:
    https://github.com/kristrev/qmi-dialer
    https://github.com/andersson/qrtr
*/

/* How should it work...
 *
 *  Connect to QMI via IPC to ATCOP service
 *  Send commands to register in the DSP and set callbacks
 *  that will be called when a cmd is executed
 *  Do something with those callbacks
 *  profit?
 *
 */

#define MAX_ATCOP_MSG_SIZE 512
#define ATCOP_REGISTER_AT_COMMAND 0x0020
#define DPM_REQUEST_OPEN_PORT 0x0020

#define ATCOP_REGISTER_AT_COMMAND_RESPONSE 0x0022

char * find_common_name(int service) {
  int i;
  for (i = 0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
    if (common_names[i].service == service) {
      return common_names[i].name;
    }
  }
  return "Unknown svc";
}

int open_socket(struct qmi_device * qmisock, uint8_t service, uint8_t instance) {
  qmisock -> fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  qmisock -> service = service;
  //sa_data="\x00\x00\x02\x39\x62\x7f\x03\x00\x00\x00\x1b\x00\x00\x00\xa0\x39\x62\x7f"
  //sa_data="\x00\x00\x01\x00\x00\x00\x03\x00\x00\x00\x1b\x00\x00\x00\x00\x00\x00\x00"}, 20
  //         \x00\x00\x01\x00\x00\x00\x08\x00\x00\x00\x1c\x00\x00\x00\x00\x00\x00\x00
  qmisock -> transaction_id = 1;
  qmisock -> socket.family = IPC_ROUTER;
  qmisock -> socket.address.addrtype = IPC_ROUTER_ADDRTYPE;
  qmisock -> socket.address.addr.port_name.service = service;
  qmisock -> socket.address.addr.port_name.instance = instance;
  qmisock -> socket.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
  qmisock -> socket.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
  return qmisock -> fd;
}

static ssize_t read_data(struct qmi_device * qmid) {
  uint8_t numbytes;
  qmux_hdr_t * qmux_hdr;

  numbytes = recvfrom(qmid -> fd, & qmid -> buf, sizeof(qmid -> buf), MSG_DONTWAIT, (void * ) & qmid -> socket, sizeof(qmid -> socket));
  if (numbytes >= sizeof(qmux_hdr_t)) {
    printf("We got a header with data\n");
    qmux_hdr = (qmux_hdr_t * ) qmid -> buf;
  }
  return numbytes;
}

int main(int argc, char ** argv) {
  printf("Let's play!\n");
  int ret;
  char message[256];
  struct qmi_device * qmidev;
  qmidev = calloc(1, sizeof(struct qmi_device));
  fprintf(stdout, "Opening socket...\n");
  ret = open_socket(qmidev, 8, 1); // open ATCop service
  if (ret < 0) {
    fprintf(stderr, "Error opening socket \n");
    return 1;
  }
  fprintf(stdout, "Sending sync...\n");
  if (qmi_ctl_send_sync(qmidev) < 0) {
    fprintf(stderr, " -- Error sending sync\n");
  }
  fprintf(stdout, "About to register commands....\n");
  struct atcmd_reg_request *atcmd;
  atcmd->ctlid = 0x00;
  atcmd->transaction_id = htole16(0);
  atcmd->msgid = htole16(32);
  atcmd->length = 0;
  atcmd->dummy1 = 0x0001;
  atcmd->dummy2 = 0x0001;
  atcmd->dummy3 = 0x0001;
  atcmd->var1 = 0x0a; // these are the ones that change between commands
  atcmd->var2 = 0x07;
  int tid = 0;
  int sckret;
  uint8_t * atptr;
  int j;
//  memset(atcmd->fillzero, 0x00, 47);
for (j = 0; j <47; j++) {
    atcmd->fillzero[j] = 0x00;
}
// (sizeof(at_commands) / sizeof(at_commands[0]))
  for (j = 0; j < 20; j++) {
    printf("Pushing command %i: %s \n", at_commands[j].command_id, at_commands[j].cmd);
    atcmd->command = (char*)malloc(sizeof(char)* strlen(at_commands[j].cmd)); 
    strncpy(&atcmd->command, at_commands[j].cmd, strlen(at_commands[j].cmd));
    atcmd->length = strlen(at_commands[j].cmd) + sizeof(struct atcmd_reg_request) - (3*sizeof(uint16_t)) - sizeof(uint8_t);
    atcmd->transaction_id = htole16(tid);
    tid++;
    sckret = sendto(qmidev -> fd, atcmd, strlen(at_commands[j].cmd) + sizeof(struct atcmd_reg_request) -4, MSG_DONTWAIT, (void * ) & qmidev -> socket, sizeof(qmidev -> socket));
    fprintf(stdout, "Response was %i\n", sckret);
   // free(atcmd->command);
  }

  close(qmidev->fd);
  return 0;
}