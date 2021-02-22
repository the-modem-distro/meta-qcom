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

char *find_common_name(int service) {
    int i;
    for (i = 0; i <(sizeof(common_names) / sizeof(common_names[0])); i++) {
        if (common_names[i].service == service) {
            return common_names[i].name;
        }
    }
    return "Unknown svc";
}

struct qmi_device open_socket(uint8_t service, uint8_t instance) {
    struct qmi_device qmisock;
    qmisock.fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    qmisock.service = service;

    qmisock.transaction_id = 1;
    qmisock.socket.family = IPC_ROUTER;
    qmisock.socket.address.addrtype = IPC_ROUTER_ADDRTYPE;
    qmisock.socket.address.addr.port_name.service = service;
    qmisock.socket.address.addr.port_name.instance = instance;
    qmisock.socket.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
    qmisock.socket.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
    return qmisock;
}


static ssize_t read_data(struct qmi_device *qmid){
    uint8_t numbytes;
    qmux_hdr_t *qmux_hdr;

    numbytes = recvfrom(qmid->fd , &qmid->buf, sizeof(qmid->buf), MSG_DONTWAIT, (void*) &qmid->socket, sizeof(qmid->socket));
    if(numbytes >= sizeof(qmux_hdr_t)) {
        printf("We got a header with data\n");
        qmux_hdr = (qmux_hdr_t*) qmid->buf;
    }
    return numbytes;
}
int main (int argc, char **argv) {
 printf("Let's play!\n");
 int i;
 struct timeval tv;
 tv.tv_sec = 1;
 tv.tv_usec = 0;
 struct qmi_device* services = malloc((sizeof(common_names) / sizeof(common_names[0])) * sizeof *services);
 for (i = 0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
     printf("* Opening socket for %s...", common_names[i].name);
     services[i] = open_socket(common_names[i].service,1);
     if (services[i].fd < 0) {
         printf("Failed! \n");
     } else {
         printf("OK! \n");
        if (i == 0) {
        system("echo -- IOCTL > /dev/kmsg");
        if(ioctl(services[i].fd, IOCTL_BIND_TOIPC, NULL) < 0) {
            printf("Error binding to IPC control port... \n");
        } else {
            printf("Successfully bind to ipc control in %s", common_names[i].name);
        }
        system("echo -- ioctl end > /dev/kmsg");
        }
         if (qmi_ctl_send_sync(&services[i]) < 0) {
             printf ("   --> Error sending sync to %s, closing this socket \n", common_names[i].name);
             if (i != 0) {
                 close(services[i].fd);
                 services[i].fd = -1;
             }
         }
     }
 }

printf ("First pass finished, let's see what remains: \n");
printf ("ID     ACTIVE    NAME\n");
printf ("---------------------------------\n");
for (i=0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
    printf("%04i ", common_names[i].service);
    if (services[i].fd > 0)
        printf("   Y    ");
    else
        printf("   N    ");
    printf(" %s \n", common_names[i].name);
}
printf("\n------\n");
int max_fd = 100;
int pret, ret,j;
unsigned long count = 0;
fd_set readfds;
uint8_t buf[8192];
system("echo ----- start loop ----- > /dev/kmsg");
//while (1) {
    printf (" --> Count: %ld", count);
	FD_ZERO(&readfds);
    for (i=0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
        if (services[i].fd > 0) {
            FD_SET(services[i].fd, &readfds);
        }
    }
    pret = select(max_fd, &readfds, NULL, NULL, &tv);
    for (i=0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
        if (services[i].fd > 0 && FD_ISSET(services[i].fd, &readfds)) {
            printf("%s FD has pending data: \n", find_common_name(services[i].service));
        	//ret = read(services[i].fd , &buf, 8192);
            ret = read_data(&services[i]);
        	/*ret = recvfrom(services[i].fd , &services[i].buf, sizeof(services[i].buf), MSG_DONTWAIT, (void*) &services[i].socket, sizeof(services[i].socket));*/
            printf("Got %i bytes back\n", ret);
            for (j=0; j < ret; j++) {
                printf ("0x%02x ", services[i].buf[j]);
            }
            printf("\n");
        
      
           parse_qmi(buf);
        }
    }
    printf(" End of round %i\n", count);
    count++;
//} //Do only one pass to clear
printf("checkpoint, regpar \n");
struct atcmd_reg_request atcmd;
atcmd.ctlid = 0x00;
atcmd.transaction_id = htole16(0);
atcmd.msgid = htole16(2);
atcmd.length = 0;
atcmd.dummy1 = 0x0001;
atcmd.dummy2 = 0x0001;
atcmd.dummy3 = 0x0001;
atcmd.var1 = 0x0a; // these are the ones that change between commands
atcmd.var2 = 0x07;
int tid = 0;
int sckret;
memset(atcmd.fillzero, 0x00, sizeof(atcmd.fillzero));
//atcmd.fillzero = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
for (i=0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
    if (services[i].service == 8) { // ATCop
     for (j=0; j <  (sizeof(at_commands) / sizeof(at_commands[0])); j++) {
         printf ("Pushing command %i: %s \n", at_commands[j].command_id, at_commands[j].cmd);
         atcmd.atcmd = calloc(1, sizeof(at_commands[j].cmd));
         strncpy(atcmd.atcmd, at_commands[j].cmd, sizeof(at_commands[j].cmd));
         atcmd.transaction_id = htole16(tid);
         tid++;
//         sckret = sendto(services[i].fd, (void *)atcmd, sizeof(atcmd) +1, MSG_DONTWAIT, (void*) &services[i].socket, sizeof(services[i].socket));

     }
    }

}

return 0;   
}

/*
Raw packets go like this on first pass. Pretty sure that 0x27 means is a sync packet but I'm either not trimming something on the beginning or not padding to skip some kind of envelope from using a socket instead of a file... or not binding? There's a lot I don't know yet
ctl  <-txnid-> <-msgid-> <-lengt->
0x02 0x00 0x00 0x00 0x00 0x07 0x00 0x02 
0x04 0x00 0x01 0x00 0x01 0x00 0x00 0x00 
0x00 0x00 0x00 0x01 0x27 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x00 0x00 0x00 

*/

/* data40 open request goes like this if transaction is == 16
ctl    <-txnid->   <-msgid->   <-lengt->
0x00, 0x01, 0x00, 0x21, 0x00, 0x1c, 0x00, 0x10,
                         D     A     T      A
0x0d, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41,
  4     0     _     C     N    T     L 
0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x11, 
0x09, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 
0x00, 0x00, 0x00 };

If tid is == 8
ctl   tid   <-msgid->   <-length->  <--- and this?
0x00, 0x01, 0x00, 0x21, 0x00, 0x1c, 0x00, 0x10,
--qmux_header?------?>  D     A     T      A
0x0d, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41,
  4     0     _     C     N    T     L 
0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x11, 
0x09, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 
0x00, 0x00, 0x00 };

qmi_header_ctl: 
0x00 ] 8t control_flags
0x01 ] 16t transaction_id
0x00 ] 16t cont...
0x21 ] 16t message_id
0x00 ] 16t cont...
0x1c ] length
--
qmux_header:
0x00 ] 16t len
0x10 ] 16t cont...
0x0d ] 8t ctl_flags
0x00 ] 8t service_type
0x01 ] Client_id? Port 1b?
0x0b ]
[DATA40_CNTL] // MSG
0x11 ] message_length? nope,second pkt is 0x05, that is where they divert
0x09 ] no idea what's all of this
0x00
0x01
0x05
0x00
0x00
0x00
0x08
0x00
0x00
0x00

From ghidra, it should be something like this but doesnt quite match:

struct {
    uint8_t is_valid_ctl_list = true;
    uint32_t ctl_list_length = 1;
    struct port_list: {
        char *port_name = "DATA40_CNTL";
        struct ep_id {
            uint8_t ep_type = 0x05 // bamdmux
            uint32_t iface_id = ?? 1?
        },
    },
    uint8_8 hw_port_list_valid = true,
    uint32_t hw_port_list_length = 1;
    arr hw_port_list[] = {
        ep_type = 0x05,
        interfaceid = 0x01?
        ep_pair = {
            consumer_pipes = 6 ? 
            producer_pipes = 6 ? That's what the kernel says...
        }
    },
    uint8_t sw_port_list_valid = false, //all set to 0 at this point...
    uint32_t sw_port_list_length = 0;
    arr sw_port_list[],
}

Second packet looks more like it:
<-- qmi_ctl_header --------------->  <--qmux_hdr
 ctl   [tid     ]   [ msg id ] [len]|[length  ]
 0x00, 0x02, 0x00, 0x20, 0x00, 0x2c, 0x00, 0x10,
 -----qmux_hdr ------>  <-------MSG -------------
 [ctl] [svc] [16t cli?]    D     A    T     A 
 0x15, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41, 
 -------------------------------------------------
   4    0     _      C     N     T     L   BAM_DMUX default_ep
 0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x05, 
 ifid  hwva  hwl
 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x11, 
 
 0x11, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 
 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 
 0x00, 0x00, 0x00




ep_id_type {
    ep_type ENUM 0x00 - 0x05,
    uint32_t iface_id,
}


ctrl_port_map_arr {
    char *port_name, [DATA40_CNTL || DATA0_CNTL || ....]
    data_ep_id default_ep [BAM_dmux, 0?]
}
hw_port_details {
    ep_id_type ep_id,
    ep_pair_type {
        uint32_t consumer_pipe_num
        uint32_t producer_pipe_num
    },
}

dpm_request_message {
    uint8_t valid_ctl_list = true | false,
    uint32_t ctl_list_length,
    ctl_port_map_arr port_list[maxsize],

    uint8_t hw_port_list_valid: true |false
    uint32_t hw_port_list_length,
    hw_port_details port_details,

    uint8_t sw_dataport_valid = true | false,
    uint32_t sw_dataport_length,
    sw_dataport_map {
        ep_id_type ep_id,
        char *port_name,
    }
}


*/
