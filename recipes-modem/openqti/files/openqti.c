#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include "openqti.h"

/*
 *Dead simple QTI daemon
 */

/*
 *	Dump packet to stdout
 *
 */
void dump_packet(char *direction, char *buf) {
	int i, linec = 0;
	printf("%s\n", direction);
	for (i = 0; i < sizeof(buf); i++) {
		printf("0x%x ", buf[i]);
		linec++;
		if (linec > 20) {
			printf("\n");
			linec = 0;
		}
	}
}

/*
 *	WIP: Enable or disable if second byte of the packet matches
 *
 */
void handle_pkt_action(char *pkt) {
	/* All packets passing between rmnet_ctl and smdcntl8 seem to be 4 byte long */
	if (sizeof(pkt) > 3) {
		if (pkt[0] == 0x1) {
			if (pkt[1] == 0x40) {
				printf("Outgoing Call start\n");
			} else if (pkt[1] == 0x46) {
				printf("Incoming Call start\n");
			} else if (pkt[1] == 0x41) {
				printf("Hang up! \n");
			}
		}
	}
}

void prepare_audio() {
	printf("Initialize audio sysfs entries\n");
	/* Sleep while waiting for /dev/snd/controlC0, the DSP might need a little while to finish starting when this is called 
	 *	Once controlC0 is ready, we can open the config file snd_soc_msm_9x07_Tomtom_I2S and look for the call or VoLTE call 
	 *	verbs
	 *  Enable AFE Loopback (Quectel and Simcom always do this at boot)
	 *	:: SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX",1
	 *  Then change loop gain echo 0x100d 0 > /sys/kernel/debug/afe_loop_gain
	 *
	 *	Need to trace alsaucm_test in case it does something interesting or if the modem really needs calibration data
	 *	After alsaucm_test,
	 *	1. Mixer_open (controlC0)
	 *	2. Set mixer controls when in call:
	 * 		'SEC_AUX_PCM_RX_Voice Mixer VoLTE':1:1
	 *		'VoLTE_Tx Mixer SEC_AUX_PCM_TX_VoLTE':1:1
	 *	
	 *	Set Alsa PCMs
	 *		PlaybackPCM 4
	 *		capturePCM 4
	 *	3. Disable on hang up
	 *	? Should set sampling rate how? 
	 *		Quectel has hardcoded sysfs entries in kernel, then pieces of tinyalsa for the rest
	 			echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/mode
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/sync
				echo %d > /sys/devices/soc:sound/pcm_mode_select
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/frame
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/quant
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/data
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/rate
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/num_slots
				echo %d > /sys/devices/soc:qcom,msm-sec-auxpcm/slot_mapping
				echo %d > /sys/devices/soc:sound/quec_auxpcm_rate
	 *		Simcom does use ALSA for that with a (more or less) untouched downstream kernel
	 */


	
}

int main(int argc, char **argv) {
	/* Descriptors */
	int diagfd = -1;
	int smem_logfd = -1;
	int rmnet_ctrlfd = -1;
	int dpl_cntlfd = -1;
	int smdcntl8_fd = -1;
	int ipc_router_socket = -1;
 	struct peripheral_ep_info epinfo;
  	struct sockaddr_msm_ipc ipc_socket_addr;
	bool debug = false; // Dump usb packets
	bool bypass_mode = false; // After setting it up, shutdown USB and restart it in SMD mode
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
	int c;
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
	printf("OpenQTI %s \n", VERSION);
	printf("--------------\n");
	while ((c = getopt (argc, argv, "db")) != -1)
		switch (c) {
		case 'd':
			printf("Debug mode\n");
			debug = true;
			bypass_mode = false;
			break;
		case 'b':
			printf("Bypass mode\n");
			debug = false;
			bypass_mode = true;
			break;
		case '?':
			printf("Options:\n");
			printf(" -d: Debug packets passing through SMD and RMNET\n");
			printf(" -b: Bypass mode (initialize and change usb mode to SMD,BAM_DMUX)\n");
			return 1;
		default:
			debug = false;
			bypass_mode = false;
			break;
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
	do {
		/* Request dpm to open smdcntl8 via IPC router */
		ret = sendto(ipc_router_socket, &qmi_msg_01, sizeof(qmi_msg_01), 0, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
		if (ret == -1){
			printf("Failed to send request to enable smdcntl8: %i \n", ret);
		}
		ret = sendto(ipc_router_socket, &qmi_msg_02, sizeof(qmi_msg_02), MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
		if (ret == -1){
			printf("Failed to send QMI message to socket: %i \n", ret);
		}

		// Wait one second after requesting bam init...
		sleep(1);
		smdcntl8_fd = open(SMD_CNTL, O_RDWR);
		if (smdcntl8_fd < 0) {
			printf("Error opening SMD Control 8 interface \n");
		}
		// Sleep 1 second just in case it needs to loop
		sleep(1);
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

	prepare_audio();

	// Should we proxy USB?
	if (!bypass_mode) {
		while (1) {
			FD_ZERO(&readfds);
			memset(rmnetbuf, 0, sizeof(rmnetbuf));
			FD_SET(rmnet_ctrlfd, &readfds);
			FD_SET(smdcntl8_fd, &readfds);
			pret = select(max_fd, &readfds, NULL, NULL, NULL);
			if (FD_ISSET(rmnet_ctrlfd, &readfds)) {
				ret = read(rmnet_ctrlfd, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0) {
					handle_pkt_action(rmnetbuf);
					psize = write(smdcntl8_fd, rmnetbuf, ret);
					if (debug) {
						dump_packet("rmnet_ctrl --> smdcntl8", rmnetbuf);
					}
				}
			} else if (FD_ISSET(smdcntl8_fd, &readfds)) {
				ret = read(smdcntl8_fd, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0)	{
					handle_pkt_action(rmnetbuf);
					psize = write(rmnet_ctrlfd, rmnetbuf, ret);
					if (debug) {
						dump_packet("smdcntl8 --> rmnet_ctrl", rmnetbuf);
					}
				}
			}
		}	
	} // If we should proxy USB
  close(smdcntl8_fd);
  close(rmnet_ctrlfd);
  close(diagfd);
  close(ipc_router_socket);
  close(dpl_cntlfd);
  if (bypass_mode) {
  	system("echo 0 > /sys/class/android_usb/android0/enable");
  	system("echo SMD,BAM_DMUX > /sys/class/android_usb/android0/f_rmnet/transports");
  	sleep(5);
  	system("echo 1 > /sys/class/android_usb/android0/enable");
  }
  return 0;
}