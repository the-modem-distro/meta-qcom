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
int handle_response(struct qmi_device *qmidev, char *buf, unsigned int sz) {
  int j, sckret;
  int packet_size;
  uint8_t cmdsize, cmdend;
  char *parsedcmd;
  int cmd_id = -1;
  struct at_command_simple_reply *cmdreply;
  cmdreply = calloc(1, sizeof(struct at_command_simple_reply));

  fprintf(stdout, "Handle response: not implemented \n");
  if (sz < 16) {
    fprintf(stderr, "Packet too small\n");
    return 0;
  }
  if (buf[0] != 0x04) {
    fprintf(stderr, "Unknown control ID!\n");
    return 0;
  }
  if (buf[3] != 0x21) {
    fprintf(stderr, "Unknown message type!\n");
    return 0;
  }

  packet_size = buf[5];
  fprintf(stdout, "Packet size is %i, received total size is %i\n", packet_size, sz);

  cmdsize = buf[18];
  cmdend = 18 + cmdsize;

  parsedcmd = calloc(cmdsize +1, sizeof(char));
  strncpy(parsedcmd, buf+19, cmdsize);

  fprintf(stdout, "Command is %s\n", parsedcmd);
  for (j = 0; j < (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
    if (strcmp(parsedcmd, at_commands[j].cmd) == 0) {
      fprintf(stdout, "Command match: %s, ID %i\n", at_commands[j].cmd, at_commands[j].command_id);
      cmd_id = at_commands[j].command_id;
    }
  }

  cmdreply->ctlid = 0x04;
  cmdreply->transaction_id = htole16(qmidev->transaction_id);
  cmdreply->msgid = htole16(33);
  cmdreply->length = sizeof(struct at_command_simple_reply)- (3*sizeof(uint16_t)) - sizeof(uint8_t);
  cmdreply->command = 0;
  cmdreply->result = 1;
  cmdreply->response = 3;
  /*
  Not expecting this to work, just finding a way of not needing
  to reboot everytime I send a command for now. I'll worry about
  correctness later */

  if (cmd_id == -1) { // Send an error
    fprintf(stderr, "Puking error as success :)\n");
    sckret = sendto(qmidev -> fd, cmdreply, sizeof(cmdreply), MSG_DONTWAIT, (void * ) & qmidev -> socket, sizeof(qmidev -> socket));

  } else {
    fprintf(stderr, "Puking success as success\n");
    sckret = sendto(qmidev -> fd, cmdreply, sizeof(cmdreply), MSG_DONTWAIT, (void * ) & qmidev -> socket, sizeof(qmidev -> socket));

    switch (cmd_id) {
      default: // Not implemented, send an OK to everything for now
        break;
    }
  }
  fprintf(stdout, "socket send res: %i\n", sckret);
  qmidev->transaction_id++;
  return 0;
}

int main(int argc, char ** argv) {
  printf("Let's play!\n");
  int ret,pret;
  fd_set readfds;
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
    atcmd = make_packet(qmidev->transaction_id, at_commands[j].cmd);
    qmidev->transaction_id++;
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
EXAMPLE RESPONSE: + HEREWEGO
read(3, 
Response header:
"\x04\x01\x00\x21\x00\x71\x00\x01\x12\x00\x01\x00\x00\x00\x01\x00\x00\x00\x09
Command sent through the AT IF
\x2b\x48\x45\x52\x45\x57\x45\x47\x4f
No clue what this is but it looks like places to get params? Like when you send qurccfg="par1","par2"?
\x10\x00\x00
\x11\x56\x00
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

EXAMPLE RESPONSE: +QWISO                
0x04 0x01 0x00 0x21 0x00 0x72 0x00 0x01 0x0f 0x00 0x01 0x00 0x00 0x00 0x05 0x00 0x00 0x00 0x06 
0x2b 0x51 0x57 0x49 0x53 0x4f 
0x10 0x04 0x00 
0x10 0x01 0x00 0x00 
0x11 0x56 0x00 0x10 0x04 0x00 0x01 0x00 0x00 0x00 
0x11 0x04 0x00 0x00 0x00 0x00 0x00 
0x12 0x04 0x00 0x0d 0x00 0x00 0x00 
0x13 0x04 0x00 0x0a 0x00 0x00 0x00 
0x14 0x04 0x00 0x00 0x00 0x00 0x00 
0x15 0x04 0x00 0x00 0x00 0x00 0x00 
0x16 0x04 0x00 0x01 0x00 0x00 0x00 
0x17 0x0c 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x18 0x0c 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x19 0x04 0x00 0x00 0x00 0x00 0x00 


struct at_command_modem_response {
	// QMI Header
	uint8_t ctlid; // 0x04
	uint16_t transaction_id; // incremental counter for each request, why 1 in its response? separate counter?
	uint16_t msgid; // 0x21 0x00// RESPONSE!
	uint16_t length; // 0x71 0x00 Sizeof the entire packet

	// the request packet itself
	// Obviously unfinished :)
	uint8_t dummy1; // always 0x00 0x01
	uint8_t var1; // 0x12?
	uint16_t dummy2; // always 0x00 0x01
	uint16_t dummy3; // always 0x00 0x00
	uint16_t dummy4; // always 0x00 0x01
  uint16_t dummy5; // 0x00 0x00
	uint8_t cmd_len; // 0x09?
  char *command;
  // The rest I have no idea yet, more sample packets are needed..
} __attribute__((packed));

*/