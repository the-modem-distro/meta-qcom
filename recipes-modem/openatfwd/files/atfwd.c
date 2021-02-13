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
#define ATCOP_REGISTER_AT_COMMAND_RESPONSE 0x0022


int connect_atcop_svc() {

}

int register_custom_commands() {

}

int callback() {
    printf(" ->Callback called\n");
}

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
    qmisock.socket.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
    qmisock.socket.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
    qmisock.socket.address.addr.port_name.service = service;	// SERVICE ID
    qmisock.socket.address.addr.port_name.instance = instance;	// Instance
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
 struct qmi_device* services = malloc((sizeof(common_names) / sizeof(common_names[0])) * sizeof *services);
 for (i = 0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
     printf("* Opening socket for %s...", common_names[i].name);
     services[i] = open_socket(common_names[i].service,1);
     if (services[i].fd < 0) {
         printf("Failed! \n");
     } else {
         printf("OK! \n");
         if (qmi_ctl_send_sync(&services[i]) < 0) {
             printf ("   --> Error sending sync to %s, closing this socket \n", common_names[i].name);
             close(services[i].fd);
             services[i].fd = -1;
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
while (1) {
    printf (" --> Count: %ld", count);
	FD_ZERO(&readfds);
    for (i=0; i < (sizeof(common_names) / sizeof(common_names[0])); i++) {
        if (services[i].fd > 0) {
            FD_SET(services[i].fd, &readfds);
        }
    }
    pret = select(max_fd, &readfds, NULL, NULL, NULL);
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
}
/*
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
                 */
return 0;   
}