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
struct atcmd_reg_request * make_packet(int tid, const char *command) {
  struct atcmd_reg_request *atcmd;
  atcmd = calloc(1, sizeof(struct atcmd_reg_request)); 

  int k;
  atcmd->ctlid = 0x00;
  atcmd->transaction_id = htole16(tid);
  atcmd->msgid = htole16(32);
  atcmd->dummy1 = 0x01;
  atcmd->dummy2 = 0x0100;
  atcmd->dummy3 = 0x00;
  atcmd->command = calloc(sizeof(char), strlen(command)+43); //WTF DOES IT NEED 43 padding zeros!
  strncpy(& atcmd->command, command, strlen(command));
  atcmd->length = strlen(command)+ 36 + sizeof(struct atcmd_reg_request) ;
  atcmd->strlength = strlen(command); 
  /* 
  VAR3 renamed to strlength, as it's always the length of the string being passed
       +1 but removing the "+" sign ($QCPWRDN...)
  VAR2:
    When string is 4 chars, it's always 0x07,
    When string is 8 chars, it's always 0x0a
    When string is 3 chars, it's always 0x06 
  VAR1 is always 3 more than VAR2
  */
  atcmd->var2 = strlen(command) +2; // It has to be something like this
  atcmd->var1 = atcmd->var2 + 3;
  return atcmd;
}
int main(int argc, char ** argv) {
  printf("Let's play!\n");
  int ret,pret;
  fd_set readfds;
  int max_fd = 100;
  char message[256];
  char buf[8192];
  struct qmi_device * qmidev;
  struct atcmd_reg_request *atcmd;
  qmidev = calloc(1, sizeof(struct qmi_device));
  fprintf(stdout, "Opening socket...\n");
  ret = open_socket(qmidev, 8, 1); // open ATCop service
  if (ret < 0) {
    fprintf(stderr, "Error opening socket \n");
    return 1;
  }

  int tid = 1;
  int count = 0;
  int sckret;
  int i,j,k;
  //struct atcmd_reg_request *atcmd;
  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    printf("Pushing command %i: %s \n", at_commands[j].command_id, at_commands[j].cmd);
    atcmd = make_packet(tid, at_commands[j].cmd);
    tid++;
    sckret = sendto(qmidev -> fd, atcmd, strlen(at_commands[j].cmd) + sizeof(struct atcmd_reg_request) +43, MSG_DONTWAIT, (void * ) & qmidev -> socket, sizeof(qmidev -> socket));
    fprintf(stdout, "Response was %i\n", sckret);
 //  ret =recvfrom(qmidev->fd, buf, sizeof(buf), MSG_DONTWAIT,  & qmidev -> socket, sizeof(struct sockaddr_msm_ipc));
    ret = read(qmidev->fd, &buf, 8192);
  // We should receive something like this "\x02\x04\x00\x20\x00\x07\x00\x02\x04\x00\x01\x00\x30\x00"
  // But we get this: 0x02   0x6b    0x00 0x20 0x00 0x07 0x00 0x02 0x04 0x00 0x01 0x00 0x01 0x00 
  //                  RESP  cmd id                                                      ^^  
  //                                                                                  fails?
  printf("Got %i bytes back: ", ret);
  if (ret > 0) {
    for (i=0; i < ret; i++) {
        printf ("0x%02x ", buf[i]);
    }
    printf("\n");
  }
  //free(atcmd);
  }

  while (1) {
    printf (" --> Count: %ld\n", count);
	  FD_ZERO(&readfds);
    FD_SET(qmidev->fd, &readfds);
    pret = select(max_fd, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(qmidev->fd, &readfds)) {
        printf("%s FD has pending data: \n", __func__);
        ret = read(qmidev->fd, &buf, 8192);
        printf("Got %i bytes back\n", ret);
        if (ret > 0) {
          for (j=0; j < ret; j++) {
              printf ("0x%02x ", buf[j]);
          }
          printf("\n");
          printf(" End of round %i\n", count);
        }
    }
    count++;
    //sleep(10);
} 
  close(qmidev->fd);
  return 0;
}