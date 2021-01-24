#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <stddef.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <termios.h>
#include <sys/signal.h>
#include "openqti.h"
#include <sys/time.h>
#include <sys/types.h>

/*
 *Dead simple QTI daemon
 */

int main(int argc, char *argv[]) {
	/* Descriptors */
	int diagfd = -1;
	int smem_logfd = -1;
	int rmnet_ctrlfd = -1;
	int dpl_cntlfd = -1;
	int smdcntl8_fd = -1;
	int ipc_router_socket = -1;
 	struct peripheral_ep_info epinfo;
  	struct sockaddr_msm_ipc ipc_socket_addr;
	bool debug = false;

	char buf[4096] = "\0";
	char diagbuf[TRACED_DIAG_SZ];
	char rmnetbuf[MAX_PACKET_SIZE] = "\0";

	/* QMI messages to request opening smdcntl8 */
  	char qmi_msg_01[] = { 0x00, 0x01, 0x00, 0x21, 0x00, 0x1c, 0x00, 0x10, 0x0d, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41, 0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x11, 0x09, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 };
	char qmi_msg_02[] = { 0x00, 0x02, 0x00, 0x20, 0x00, 0x2c, 0x00, 0x10, 0x15, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41, 0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	char bufsck[4096];
	unsigned int rmtmask = 0;	// mask looks like 0 in ghidra, and looks like 0 from the strace too
	int ret;
	int pret;
	int linestate;
	int max_fd = 20;
	int psize;
  	fd_set readfds;
	
	/* IPC Socket settings */
	ipc_socket_addr.family = IPC_ROUTER;
	ipc_socket_addr.address.addrtype = IPC_ROUTER_ADDRTYPE;
	ipc_socket_addr.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
	ipc_socket_addr.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
	ipc_socket_addr.address.addr.port_name.service = 0x2f;	// SERVICE ID
	ipc_socket_addr.address.addr.port_name.instance = 0x1;	// Instance

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	epinfo.ep_type = DATA_EP_TYPE_BAM_DMUX;
	epinfo.peripheral_iface_id = RMNET_CONN_ID;
	/* The modem needs some more time to boot the DSP
	 * Since I haven't found a way to query its status
	 * except by probing it, I loop until the IPC socket
	 * manages to open /dev/smdcntl8, which is the point
	 * where it is ready
	 */
	printf("OpenQTI! \n");
	if (argc > 1) {
		if (strcmp(argv[1], "-d") == 0) {
			debug = true;
		}
	}
	if (diagfd < 0) {
    	printf("Opening DIAG interface \n");
    	diagfd = open(DIAGDEV, O_RDWR);
	}

	if (ioctl(diagfd, _IOC(_IOC_NONE, 0, 0x20, 0), &ret) < 0)	{
		printf("IOCTL to diag failed \n");
	}

	/* Diag always return the same data on init, and it
	 * stops with a big chunk. Just replicating what QTI
	 * does, though it's probably unnecessary
	 */
	do {
		memset(buf, 0, TRACED_DIAG_SZ);
		ret = read(diagfd, diagbuf, TRACED_DIAG_SZ);
	} while (ret != 8232);

  if (rmnet_ctrlfd < 0)	{
    printf("Opening RMNET_CTRL \n");
    rmnet_ctrlfd = open(RMNET_CTL, O_RDWR);
    }

	/* Sleep just a bit to let it breathe */
	sleep(1);
 	if (ipc_router_socket < 0) {
      printf("Opening socket to the IPC Router \n");
      ipc_router_socket = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    }

	if (ioctl(ipc_router_socket, _IOC(_IOC_READ, IPC_IOCTL_MAGIC, 0x4, 0x4), 0) < 0) {
		printf("IOCTL to the IPC1 socket failed \n");
	}

    if (dpl_cntlfd < 0) {
      printf("Opening DPL_CTRL interface \n");
  	  dpl_cntlfd = open(DPL, O_RDWR);
	}
	// Unknown IOCTL, just before line state to rmnet
	if (ioctl(dpl_cntlfd, _IOC(_IOC_READ, 0x72, 0x2, 0x4), &ret) < 0)	{
		printf("IOCTL failed: %i\n", ret);
	}

	if (ioctl(rmnet_ctrlfd, EP_LOOKUP, &epinfo) < 0) {
		printf("EP_Lookup failed \n");
	} 

	ret = setsockopt(ipc_router_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret) {
		printf("Error setting socket options \n");
	}
	ret = bind(ipc_router_socket, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
	if (ret < 0) {
		printf("Error binding to socket");
	}
	do {
	/* Request opening the socket */
	ret = sendto(ipc_router_socket, &qmi_msg_01, sizeof(qmi_msg_01), 0, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
	printf("Data40 sendto: ret: %i", ret);

	ret = recvfrom(ipc_router_socket, &bufsck, 3156, MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
	printf("Receive from IPC: %i, %s", ret, bufsck);

	printf("Send QMI Message 2\n");
	ret = sendto(ipc_router_socket, &qmi_msg_02, sizeof(qmi_msg_02), MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
	if (ret == -1){
		printf("Failed to send QMI message to socket: %i \n", ret);
	}

	ret = recvfrom(ipc_router_socket, &bufsck, 2048, MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
	printf("Receive from IPC: %i, %s", ret, bufsck);
	if (ret == -1)	{
		printf("Failed to receive QMI message to socket: %i \n", ret);
	}
  	// Wait one second after requesting bam init...
	sleep(1);
	smdcntl8_fd = open(SMD_CNTL, O_RDWR);
	if (smdcntl8_fd < 0) {
		printf("Error opening SMD Control 8 interface \n");
	}
	// Sleep 2 seconds just in case it needs to loop
	sleep(2);
  } while (smdcntl8_fd < 0);

	printf("SMD Interface opened \n");
	int dtrsignal = (TIOCM_DTR | TIOCM_RTS | TIOCM_CD);
	if ((ioctl(smdcntl8_fd, TIOCMSET, (void*) &dtrsignal)) == -1)	{
		printf("Failed to set DTR high \n");
		return -1;
	}

	/* QTI gets line state, then sets modem offline and online while initializing */
	ret = ioctl(rmnet_ctrlfd, GET_LINE_STATE, &linestate);
	printf("Get line  RMNET state: %i, %i \n", linestate, ret);

	ret = ioctl(rmnet_ctrlfd, MODEM_OFFLINE);
	printf("Set modem offline: %i \n", ret);

	ret = ioctl(rmnet_ctrlfd, MODEM_ONLINE);
	printf("Set modem online (set DTR high): %i \n", ret);

	while (1) {
		FD_ZERO(&readfds);
		memset(rmnetbuf, 0, sizeof(rmnetbuf));
		FD_SET(rmnet_ctrlfd, &readfds);
		FD_SET(smdcntl8_fd, &readfds);
			pret = select(max_fd, &readfds, NULL, NULL, NULL);
			if (FD_ISSET(rmnet_ctrlfd, &readfds)) {
				printf("Packet received in rmnet \n");
				ret = read(rmnet_ctrlfd, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0) {
					if (debug) {
						printf("--> rmnet_ctl: SZ:%i: str: %s \n ", rmnetbuf, ret, sizeof(rmnetbuf));
					}
					psize = write(smdcntl8_fd, rmnetbuf, ret);
				}
			} else if (FD_ISSET(smdcntl8_fd, &readfds)) {
				printf("Packet received in smdcntl8 \n");
				ret = read(smdcntl8_fd, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0)	{
					if (debug) {
						printf("--> smdcntl8: SZ:%i: str: %s \n ", rmnetbuf, ret, sizeof(rmnetbuf));
					}
					psize = write(rmnet_ctrlfd, rmnetbuf, ret);
				}
			}
	}

  close(smdcntl8_fd);
  close(rmnet_ctrlfd);
  close(diagfd);
  close(ipc_router_socket);
  close(dpl_cntlfd);
  return 0;
}