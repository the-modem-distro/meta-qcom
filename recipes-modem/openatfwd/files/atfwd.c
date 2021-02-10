#include <unistd.h>
#include <stdint.h>
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

struct qmi_device open_socket(uint8_t service, uint8_t instance) {
    struct qmi_device qmisock;
    qmisock.fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    qmisock.transaction_id = 1;
    qmisock.socket.family = IPC_ROUTER;
    qmisock.socket.address.addrtype = IPC_ROUTER_ADDRTYPE;
    qmisock.socket.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
    qmisock.socket.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
    qmisock.socket.address.addr.port_name.service = service;	// SERVICE ID
    qmisock.socket.address.addr.port_name.instance = instance;	// Instance
    return qmisock;
}

int main (int argc, char **argv) {
 printf("Let's play!\n");
 int i;
 struct qmi_device* services = malloc(sizeof(common_names) * sizeof *services);
 for (i = 0; i < sizeof(common_names); i++) {
     printf("%s: Opening socket for service %i: %s...",__func__, common_names[i].service, common_names[i].name);
     services[i] = open_socket(common_names[i].service,1);
     if (services[i].fd < 0) {
         printf("Failed! \n");
     } else {
         printf("OK! \n");
         qmi_ctl_send_sync(services[i]);
     }
 }

printf("Eeeend \n");
return 0;   
}

