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

int main (int argc, char **argv) {
 printf("Let's play!\n");
int ipc_router_socket = -1;
struct peripheral_ep_info epinfo;
struct sockaddr_msm_ipc ipc_socket_addr;
struct timeval tv;

/* IPC Socket settings */
ipc_socket_addr.family = IPC_ROUTER;
ipc_socket_addr.address.addrtype = IPC_ROUTER_ADDRTYPE;
ipc_socket_addr.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
ipc_socket_addr.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
ipc_socket_addr.address.addr.port_name.service = 0x2f;	// SERVICE ID
ipc_socket_addr.address.addr.port_name.instance = 0x1;	// Instance

epinfo.ep_type = DATA_EP_TYPE_BAM_DMUX;
epinfo.peripheral_iface_id = RMNET_CONN_ID;

if (ipc_router_socket < 0) {
    fprintf(stdout,"Opening socket to the IPC Router \n");
    ipc_router_socket = socket(IPC_ROUTER, SOCK_DGRAM, 0);
}

if (ioctl(ipc_router_socket, _IOC(_IOC_READ, IPC_IOCTL_MAGIC, 0x4, 0x4), callback()) < 0) {
    fprintf(stderr,"IOCTL to the IPC1 socket failed \n");
}


printf("Eeeend \n");
return 0;   
}