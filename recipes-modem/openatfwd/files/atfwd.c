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

int open_socket(struct qmi_device * qmisock, uint8_t service, uint8_t instance) {
  qmisock -> fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  qmisock -> service = service;
  qmisock -> transaction_id = 1;
  qmisock -> socket.family = IPC_ROUTER;
  qmisock -> socket.address.addrtype = IPC_ROUTER_ADDRTYPE;
  qmisock -> socket.address.addr.port_name.service = service;
  qmisock -> socket.address.addr.port_name.instance = instance;
  qmisock -> socket.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
  qmisock -> socket.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
  return qmisock -> fd;
}

struct atcmd_reg_request * make_packet(int tid, const char *command) {
  struct atcmd_reg_request *atcmd;
  int k;

  atcmd = calloc(1, sizeof(struct atcmd_reg_request)); 
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

int handle_response(struct qmi_device *qmidev, char *buf, unsigned int sz) {
  fprintf(stdout, "Handle response: not implemented \n");
  printf("%s\n", buf);
  return 0;
}

int main(int argc, char ** argv) {
  printf("Let's play!\n");
  int ret,pret;
  fd_set readfds;
  int tid = 1; // Transaction ID, to keep track of the commands sent
  int sckret;
  int i,j;
  int max_fd = 100; // max ID of FDs to track in select()
  char buf[MAX_PACKET_SIZE];
  struct qmi_device * qmidev;
  struct atcmd_reg_request *atcmd;
  qmidev = calloc(1, sizeof(struct qmi_device));
  fprintf(stdout, "Opening socket...\n");
  ret = open_socket(qmidev, 8, 1); // open ATCop service
  if (ret < 0) {
    fprintf(stderr, "Error opening socket \n");
    return 1;
  }


  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    printf("Pushing command %i: %s \n", at_commands[j].command_id, at_commands[j].cmd);
    atcmd = make_packet(tid, at_commands[j].cmd);
    tid++;
    sckret = sendto(qmidev -> fd, atcmd, strlen(at_commands[j].cmd) + sizeof(struct atcmd_reg_request) +43, MSG_DONTWAIT, (void * ) & qmidev -> socket, sizeof(qmidev -> socket));
    if (sckret < 0) {
      fprintf(stderr, "Failed to send command %s \n", at_commands[j].cmd);
    }
    ret = read(qmidev->fd, &buf, MAX_PACKET_SIZE);
    if (ret < 0) {
      fprintf(stderr, "Modem rejected command %s \n", at_commands[j].cmd);
    }
    sleep(0.1);
  }

  while (1) {
	  FD_ZERO(&readfds);
    FD_SET(qmidev->fd, &readfds);
    pret = select(max_fd, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(qmidev->fd, &readfds)) {
        ret = read(qmidev->fd, &buf, MAX_PACKET_SIZE);
        handle_response(qmidev, buf, ret);
        printf("Got %i bytes back\n", ret);
        if (ret > 0) {
          for (j=0; j < ret; j++) {
              printf ("0x%02x ", buf[j]);
          }
          printf("\n");
        }
    }
} 
  close(qmidev->fd);
  return 0;
}

/*
EXAMPLE RESPONSE
read(3, 
Response header:
"\x04\x01\x00\x21\x00\x71\x00\x01\x12\x00\x01\x00\x00\x00\x01\x00\x00\x00\x09
Command sent through the AT IF
\x2b\x48\x45\x52\x45\x57\x45\x47\x4f
No clue what this is but it looks like places to get params? Like when you send qurccfg="par1","par2"?
\x10\x00\x00\x11\x56\x00
\x10\x04\x00\x01\x00\x00\x00
\x11\x04\x00\x00\x00\x00\x00
\x12\x04\x00\x0d\x00\x00\x00
\x13\x04\x00\x0a\x00\x00\x00
\x14\x04\x00\x00\x00\x00\x00
\x15\x04\x00\x00\x00\x00\x00
\x16\x04\x00\x01\x00\x00\x00
\x17\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
\x18\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
\x19\x04\x00\x00\x00\x00\x00", 8192) = 120

*/