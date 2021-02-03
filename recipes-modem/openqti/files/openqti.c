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
#include "openqti.h"
#include "audio.h"
#include <errno.h>

/*
 *Dead simple QTI daemon
 */

/* Outgoing
smdcntl8 --> rmnet_ctrl :0x1 0x10 0x0 0x80 
smdcntl8 --> rmnet_ctrl :0x1 0x40 0x0 0x80 
smdcntl8 --> rmnet_ctrl :0x1 0x3a 0x0 0x80 
*/
/* Hangup
smdcntl8 --> rmnet_ctrl :0x1 0x41 0x0 0x80 
rmnet_ctrl --> smdcntl8 :0x1 0xc 0x0 0x0 
*/

/* Incoming
smdcntl8 --> rmnet_ctrl :0x1 0x10 0x0 0x80 
smdcntl8 --> rmnet_ctrl :0x1 0x46 0x0 0x80 
smdcntl8 --> rmnet_ctrl :0x1 0x4c 0x0 0x80 
*/

void dump_packet(char *direction, char *buf) {
	int i;
	fprintf(stdout,"%s :", direction);
	for (i = 0; i < sizeof(buf); i++) {
		fprintf(stdout,"0x%02x ", buf[i]);
	}
	fprintf(stdout,"\n");
}
char prev_pkt;
/* Called from the select() */
void handle_pkt_action(char *pkt, int from) {
	/* All packets passing between rmnet_ctl and smdcntl8 seem to be 4 byte long */
	/* This is messy and doesn't work correctly. Sometimes there's a flood and this gets
	   hit leaving audio on for no reason, need to find a reliable way of knowing if 
	   there's an actual call */
	if (sizeof(pkt) > 3 && from == FROM_DSP) {
		if (pkt[0] == 0x1) {
			if (prev_pkt == 0x40 && pkt[1] == 0x3a) {
				fprintf(stdout,"Outgoing Call start\n");
				start_audio();
			} else if (prev_pkt == 0x46 && pkt[1] == 0x4c) {
				fprintf(stdout,"Incoming Call start : voice\n");
				start_audio();
			} else if (prev_pkt == 0x3c && pkt[1] == 0x42) {
				fprintf(stdout,"Incoming Call start : VoLTE\n");
				start_audio();
			} else if (pkt[1] == 0x41 || pkt[1] == 0x69) {
				fprintf(stdout,"Hang up! \n");
				stop_audio();
			}
		}
	prev_pkt = pkt[1];
	}

}


int set_mixer_ctl(struct mixer *mixer, char *name, bool enable) {
	struct mixer_ctl *ctl;
	mixer_dump_by_id(mixer, name);
	ctl = get_ctl(mixer, name);
	int r;
	if (enable) {
		fprintf(stderr," --> Setting %s to ENABLE... \n", name);
		r = mixer_ctl_set_value(ctl, 1, 1);
		if (r < 0) {
			fprintf(stderr,"Failed! %i\n", r);
		} else {
			fprintf(stderr," Success: %i\n", r);
		}
	} else {
		fprintf(stderr," --> Setting %s to DISABLE\n", name);
		r = mixer_ctl_set_value(ctl, 1, 0);
		if (r < 0) {
			fprintf(stderr,"Failed! \n");
		} else {
			fprintf(stderr," Success! \n");
		}

	}
	return 0;
}

int stop_audio() {
	if (!is_call_active) {
		fprintf(stderr,"%s: No call in progress \n", __func__);
		return 1;
	}

	if (pcm_tx->fd >= 0)
		pcm_close(pcm_tx);
	if (pcm_rx->fd >= 0)
		pcm_close(pcm_rx);

	mixer = mixer_open(SND_CTL);
    if (!mixer){
        fprintf(stderr,"error opening mixer! %s: %d\n", strerror(errno), __LINE__);
        return 0;
    }

	set_mixer_ctl(mixer, AFECTL, false);
	set_mixer_ctl(mixer, TXCTL, false); // Playback
	set_mixer_ctl(mixer, RXCTL, false); // Capture
	mixer_close(mixer);
	is_call_active = false;
	return 1;
}

int start_audio() {
	if (is_call_active) {
		fprintf(stderr,"%s: Audio already active, nothing to do\n", __func__);
		return 1;
	}

	/*
	set _enadev Auxpcm Tx" > /run/alsaucm_test
	SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX
	start voice_pcm_service_thread, ret = %d
	voice_pcm_service_thread
	SEC_AUX_PCM_RX_Voice Mixer CSVoice
	Voice_Tx Mixer SEC_AUX_PCM_TX_Voice
	SEC_AUX_PCM_RX_Voice Mixer VoLTE
	VoLTE_Tx Mixer SEC_AUX_PCM_TX_VoLTE
	AFE_PCM_RX_Voice Mixer CSVoice
	Voice_Tx Mixer AFE_PCM_TX_Voice
	AFE_PCM_RX_Voice Mixer VoLTE
	VoLTE_Tx Mixer AFE_PCM_TX_VoLTE
	*/
	mixer = mixer_open(SND_CTL);
	fprintf(stderr," Open mixer \n");
    if (!mixer){
        fprintf(stderr,"%s: Error opening mixer!\n", __func__);
        return 0;
    }
	set_mixer_ctl(mixer, AFECTL, true);
	set_mixer_ctl(mixer, TXCTL, true); // Playback
	set_mixer_ctl(mixer, RXCTL, true); // Capture

	system("echo 0 > /sys/devices/soc:qcom,msm-sec-auxpcm/mode");
	system("echo 0 > /sys/devices/soc:qcom,msm-sec-auxpcm/sync");
	system("echo 0 > /sys/devices/soc:sound/pcm_mode_select");
	system("echo 2 > /sys/devices/soc:qcom,msm-sec-auxpcm/frame");
	system("echo 1 > /sys/devices/soc:qcom,msm-sec-auxpcm/data");
	system("echo 256000 > /sys/devices/soc:qcom,msm-sec-auxpcm/rate");
	system("echo 8000 > /sys/devices/soc:sound/quec_auxpcm_rate");

	/* Now PCM */
	mixer_close(mixer);

	/* Check if PCM was already active */
	pcm_rx = pcm_open(PCM_IN | PCM_MONO,PCM_DEV);
	pcm_rx->channels = 1;
	pcm_rx->rate = 8000;
	pcm_rx->flags =  PCM_IN | PCM_MONO;

	pcm_tx = pcm_open(PCM_OUT | PCM_MONO, PCM_DEV);
	pcm_tx->channels = 1;
	pcm_tx->rate = 8000;
	pcm_tx->flags = PCM_OUT | PCM_MONO;

	if (set_params(pcm_rx, PCM_IN)) {
		fprintf(stderr,"Error setting RX Params\n");
		pcm_close(pcm_rx);
		return -EINVAL;
	}

	if (set_params(pcm_tx, PCM_OUT)) {
		fprintf(stderr,"Error setting TX Params\n");
		pcm_close(pcm_tx);
		return -EINVAL;
	} 

    if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
		fprintf(stderr,"Error getting RX PCM ready\n");
		pcm_close(pcm_rx);
		return -EINVAL;
    }

	if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
		fprintf(stderr,"Error getting TX PCM ready\n");
		pcm_close(pcm_tx);
		return -EINVAL;
    }

	if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_START) < 0) {
			fprintf(stderr,"PCM ioctl start failed for TX\n");
			pcm_close(pcm_tx);
	}

	if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_START) < 0) {
			fprintf(stderr,"PCM ioctl start failed for RX\n");
			pcm_close(pcm_rx);
	}
	
	is_call_active = true;
}

int dump_audio_mixer() {
	struct mixer *mixer;
    mixer = mixer_open(SND_CTL);
    if (!mixer){
		fprintf(stderr,"%s: Error opening mixer!\n", __func__);        
		return -1;
    }
    mixer_dump(mixer);
    mixer_close(mixer);
	return 0;
}


int main(int argc, char **argv) {
	/* Descriptors */
	int rmnet_ctrlfd = -1;
	int dpl_cntlfd = -1;
	int smdcntl8_fd = -1;
	int ipc_router_socket = -1;
 	struct peripheral_ep_info epinfo;
  	struct sockaddr_msm_ipc ipc_socket_addr;
	bool debug = false; // Dump usb packets
	bool bypass_mode = false; // After setting it up, shutdown USB and restart it in SMD mode
	char rmnetbuf[MAX_PACKET_SIZE] = "\0";

	/* QMI messages to request opening smdcntl8 */
  	char qmi_msg_01[] = { 0x00, 0x01, 0x00, 0x21, 0x00, 0x1c, 0x00, 0x10, 0x0d, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41, 0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x11, 0x09, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 };
	char qmi_msg_02[] = { 0x00, 0x02, 0x00, 0x20, 0x00, 0x2c, 0x00, 0x10, 0x15, 0x00, 0x01, 0x0b, 0x44, 0x41, 0x54, 0x41, 0x34, 0x30, 0x5f, 0x43, 0x4e, 0x54, 0x4c, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int ret;
	int pret;
	int linestate;
	int max_fd = 20;
  	fd_set readfds;
	
	/* IPC Socket settings */
	ipc_socket_addr.family = IPC_ROUTER;
	ipc_socket_addr.address.addrtype = IPC_ROUTER_ADDRTYPE;
	ipc_socket_addr.address.addr.port_addr.node_id = IPC_HEXAGON_NODE;
	ipc_socket_addr.address.addr.port_addr.port_id = IPC_HEXAGON_PORT;
	ipc_socket_addr.address.addr.port_name.service = 0x2f;	// SERVICE ID
	ipc_socket_addr.address.addr.port_name.instance = 0x1;	// Instance

	struct timeval tv;

	epinfo.ep_type = DATA_EP_TYPE_BAM_DMUX;
	epinfo.peripheral_iface_id = RMNET_CONN_ID;

	fprintf(stdout,"OpenQTI \n");
	fprintf(stdout,"--------------\n");
	while ((ret = getopt (argc, argv, "adb")) != -1)
		switch (ret) {
		case 'a':
			fprintf(stdout,"Dump audio mixer data \n");
			return dump_audio_mixer();
			break;
		case 'd':
			fprintf(stdout,"Debug mode\n");
			debug = true;
			bypass_mode = false;
			break;
		case 'b':
			fprintf(stdout,"Bypass mode\n");
			debug = false;
			bypass_mode = true;
			break;
		case '?':
			fprintf(stdout,"Options:\n");
			fprintf(stdout," -a: Dump audio mixer controls\n");
			fprintf(stdout," -d: Debug packets passing through SMD and RMNET\n");
			fprintf(stdout," -b: Bypass mode (initialize and change usb mode to SMD,BAM_DMUX, no audio support)\n");
			return 1;
		default:
			debug = false;
			bypass_mode = false;
			break;
		}

  if (rmnet_ctrlfd < 0)	{
    fprintf(stdout,"Opening RMNET_CTRL \n");
    rmnet_ctrlfd = open(RMNET_CTL, O_RDWR);
    }

	/* Sleep just a bit to let it breathe */
	sleep(1);
 	if (ipc_router_socket < 0) {
      fprintf(stdout,"Opening socket to the IPC Router \n");
      ipc_router_socket = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    }

	if (ioctl(ipc_router_socket, _IOC(_IOC_READ, IPC_IOCTL_MAGIC, 0x4, 0x4), 0) < 0) {
		fprintf(stderr,"IOCTL to the IPC1 socket failed \n");
	}

    if (dpl_cntlfd < 0) {
      fprintf(stdout,"Opening DPL_CTRL interface \n");
  	  dpl_cntlfd = open(DPL, O_RDWR);
	}
	// Unknown IOCTL, just before line state to rmnet
	if (ioctl(dpl_cntlfd, _IOC(_IOC_READ, 0x72, 0x2, 0x4), &ret) < 0)	{
		fprintf(stderr,"IOCTL failed: %i\n", ret);
	}

	if (ioctl(rmnet_ctrlfd, EP_LOOKUP, &epinfo) < 0) {
		fprintf(stderr,"EP_Lookup failed \n");
	} 

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	ret = setsockopt(ipc_router_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret) {
		fprintf(stderr,"Error setting socket options \n");
	}
	do {
		/* Request dpm to open smdcntl8 via IPC router */
		ret = sendto(ipc_router_socket, &qmi_msg_01, sizeof(qmi_msg_01), 0, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
		if (ret == -1){
			fprintf(stderr,"Failed to send request to enable smdcntl8: %i \n", ret);
		}
		ret = sendto(ipc_router_socket, &qmi_msg_02, sizeof(qmi_msg_02), MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
		if (ret == -1){
			fprintf(stderr,"Failed to send QMI message to socket: %i \n", ret);
		}

		// Wait one second after requesting bam init...
		
		sleep(1);
		smdcntl8_fd = open(SMD_CNTL, O_RDWR);
		if (smdcntl8_fd < 0) {
			fprintf(stderr,"Error opening SMD Control 8 interface \n");
		}
		// Sleep 1 second just in case it needs to loop
		sleep(1);
	/* The modem needs some more time to boot the DSP
	 * Since I haven't found a way to query its status
	 * except by probing it, I loop until the IPC socket
	 * manages to open /dev/smdcntl8, which is the point
	 * where it is ready
	 */
  } while (smdcntl8_fd < 0);
	fprintf(stdout,"SMD Interface opened \n");
	int dtrsignal = (TIOCM_DTR | TIOCM_RTS | TIOCM_CD);
	if ((ioctl(smdcntl8_fd, TIOCMSET, (void*) &dtrsignal)) == -1)	{
		fprintf(stderr,"Failed to set DTR high \n");
		return -1;
	}

	/* QTI gets line state, then sets modem offline and online while initializing */
	ret = ioctl(rmnet_ctrlfd, GET_LINE_STATE, &linestate);
	if (ret < 0)
		fprintf(stderr,"Get line  RMNET state: %i, %i \n", linestate, ret);

	ret = ioctl(rmnet_ctrlfd, MODEM_OFFLINE);
	if (ret < 0)
		fprintf(stderr,"Set modem offline: %i \n", ret);

	ret = ioctl(rmnet_ctrlfd, MODEM_ONLINE);
	if (ret < 0)
		fprintf(stderr,"Set modem online: %i \n", ret);

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
					handle_pkt_action(rmnetbuf, FROM_HOST);
					ret = write(smdcntl8_fd, rmnetbuf, ret);
					if (debug) {
						dump_packet("rmnet_ctrl --> smdcntl8", rmnetbuf);
					}
				}
			} else if (FD_ISSET(smdcntl8_fd, &readfds)) {
				ret = read(smdcntl8_fd, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0)	{
					handle_pkt_action(rmnetbuf, FROM_DSP);
					ret = write(rmnet_ctrlfd, rmnetbuf, ret);
					if (debug) {
						dump_packet("smdcntl8 --> rmnet_ctrl", rmnetbuf);
					}
				}
			}
		}	
	} // If we should proxy USB
  close(smdcntl8_fd);
  close(rmnet_ctrlfd);
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