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

void dump_packet(char *direction, char *buf) {
	int i;
	fprintf(stdout,"%s :", direction);
	for (i = 0; i < sizeof(buf); i++) {
		fprintf(stdout,"0x%02x ", buf[i]);
	}
	fprintf(stdout,"\n");
}

/* Called from the select() */
void handle_pkt_action(char *pkt, int from) {
	/* All packets passing between rmnet_ctl and smdcntl8 seem to be 4 byte long */
	/* This is messy and doesn't work correctly. Sometimes there's a flood and this gets
	   hit leaving audio on for no reason, need to find a reliable way of knowing if 
	   there's an actual call */
	if (sizeof(pkt) > 3 && from == FROM_DSP) {
		if (pkt[0] == 0x1) {
			switch (pkt[1]) {
				case 0x40:
				fprintf(stdout,"Outgoing Call start: VoLTE\n");
				start_audio(1);
				break;
				case 0x38:
				fprintf(stdout,"Outgoing Call start: CS Voice\n");
				start_audio(0);
				break;		
				case 0x46: //not really tested
				fprintf(stdout,"Incoming Call start : Voice\n");
				start_audio(1);
				break;
				case 0x3c: // not really tested
				fprintf(stdout,"Incoming Call start : VoLTE\n");
				start_audio(1);
				break;
				case 0x41:
				fprintf(stdout,"Hang up! \n");
				stop_audio();
				break;				

			}
		}
	}
}


int set_mixer_ctl(struct mixer *mixer, char *name, int value) {
	struct mixer_ctl *ctl;
	mixer_dump_by_id(mixer, name);
	ctl = get_ctl(mixer, name);
	int r;

	fprintf(stderr," --> Setting %s to value %i... \n", name, value);
	r = mixer_ctl_set_value(ctl, 1, value);
	if (r < 0) {
		fprintf(stderr,"Failed! %i\n", r);
	} else {
		fprintf(stderr," Success: %i\n", r);
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
	
	// We close all the mixers
	set_mixer_ctl(mixer, TXCTL_VOLTE, 0); // Playback
	set_mixer_ctl(mixer, RXCTL_VOLTE, 0); // Capture
	set_mixer_ctl(mixer, TXCTL_VOICE, 0); // Playback
	set_mixer_ctl(mixer, RXCTL_VOICE, 0); // Capture

	mixer_close(mixer);
	is_call_active = false;
	return 1;
}

int write_to(const char *path, const char *val) {
	int ret;
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		return ENOENT;
	}
	ret = write(fd, val, sizeof(val));
	close(fd);
	return ret;
}

/*	Setup mixers and open PCM devs
 *		type: 0: CS Voice Call
 *		      1: VoLTE Call
 */
int start_audio(int type) {
	int i;
	char pcm_device[18];
	if (is_call_active) {
		fprintf(stderr,"%s: Audio already active, restarting... \n", __func__);
		stop_audio();
	}

	mixer = mixer_open(SND_CTL);
	fprintf(stderr," Open mixer \n");
    if (!mixer){
        fprintf(stderr,"%s: Error opening mixer!\n", __func__);
        return 0;
    }

	switch (type) {
		case 0:
			set_mixer_ctl(mixer, TXCTL_VOICE, 1); // Playback
			set_mixer_ctl(mixer, RXCTL_VOICE, 1); // Capture
			strncpy(pcm_device, PCM_DEV_VOCS, sizeof(PCM_DEV_VOCS));
			break;
		case 1:
			set_mixer_ctl(mixer, TXCTL_VOLTE, 1); // Playback
			set_mixer_ctl(mixer, RXCTL_VOLTE, 1); // Capture
			strncpy(pcm_device, PCM_DEV_VOLTE, sizeof(PCM_DEV_VOLTE));
			break;
		default:
			fprintf(stderr, "%s: Can't set mixers, unknown call type %i\n", __func__, type);
			break;
	}
	mixer_close(mixer);
	printf ("Using PCM Device %s \n", pcm_device);
	
	pcm_rx = pcm_open(PCM_IN | PCM_MONO, pcm_device);
	pcm_rx->channels = 1;
	pcm_rx->rate = 8000;
	pcm_rx->flags =  PCM_IN | PCM_MONO;

	pcm_tx = pcm_open(PCM_OUT | PCM_MONO, pcm_device);
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
	bool recreate = false;
	char rmnetbuf[MAX_PACKET_SIZE] = "\0";
	int i;
	int ret;
	int pret;
	int linestate;
	int max_fd = 20;
  	fd_set readfds;
	struct portmapper_open_request dpmreq;

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

 	if (ipc_router_socket < 0) {
      fprintf(stdout,"Opening socket to the IPC Router \n");
      ipc_router_socket = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    }

	if (ioctl(ipc_router_socket, IOCTL_BIND_TOIPC, 0) < 0) {
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
		/* Parts of this are probably _very_ wrong*/
		dpmreq.ctlid = 0x00;
		dpmreq.transaction_id = htole16(1);
		dpmreq.msgid = htole16(32);
		dpmreq.length= sizeof(struct portmapper_open_request) - (3*sizeof(uint16_t)) - sizeof(uint8_t); // Size of the packet minux qmi header
		dpmreq.is_valid_ctl_list = htole16(0x10);
		dpmreq.ctl_list_length = 0x0b010015; //sizeof(SMDCTLPORTNAME); this has to be wrong...

		strncpy(dpmreq.hw_port_map[0].port_name, SMDCTLPORTNAME, sizeof(SMDCTLPORTNAME));
		dpmreq.hw_port_map[0].epinfo.ep_type = htole16(DATA_EP_TYPE_BAM_DMUX);
		dpmreq.hw_port_map[0].epinfo.peripheral_iface_id = 0x08000000; 
		
		dpmreq.is_valid_hw_list = 0x00;
		dpmreq.hw_list_length = 0x11110000;
		dpmreq.hw_epinfo.ph_ep_info.ep_type = DATA_EP_TYPE_RESERVED;
		dpmreq.hw_epinfo.ph_ep_info.peripheral_iface_id = 0x00000501;

		dpmreq.hw_epinfo.ipa_ep_pair.cons_pipe_num = 0x00000800; // 6 is what the bam responds but packet seems to say 0
		dpmreq.hw_epinfo.ipa_ep_pair.prod_pipe_num = htole32(0); 

		dpmreq.is_valid_sw_list = false;
		dpmreq.sw_list_length = 0;

		ret = sendto(ipc_router_socket, &dpmreq, sizeof(dpmreq), MSG_DONTWAIT, (void*) &ipc_socket_addr, sizeof(ipc_socket_addr));
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

	fprintf(stdout, "Setting audio sysfs entries... \n");
	for (i = 0; i < sizeof(sysfs_value_pairs); i++ ){
		if (write_to(sysfs_value_pairs[i].path, sysfs_value_pairs[i].value) <0) {
			fprintf(stderr, "Error writing to %s\n", sysfs_value_pairs[i].path);

		}
	}
	// Should we proxy USB?
	if (!bypass_mode) {
		fprintf(stdout, " Normal mode: Ready!\n");
		while (1) {
			FD_ZERO(&readfds);
			memset(rmnetbuf, 0, sizeof(rmnetbuf));
			FD_SET(rmnet_ctrlfd, &readfds);
			FD_SET(smdcntl8_fd, &readfds);
			FD_SET(ipc_router_socket, &readfds);
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
			} else if (FD_ISSET(ipc_router_socket, &readfds)) {
				ret = read(ipc_router_socket, &rmnetbuf, MAX_PACKET_SIZE);
				if (ret > 0)	{
					if (debug) {
						dump_packet("ipcrtr:", rmnetbuf);
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
	fprintf(stdout, "Bypass mode: ready!\n");
	write_to("/sys/class/android_usb/android0/enable", "0");
	write_to("/sys/class/android_usb/android0/f_rmnet/transports", "SMD,BAM_DMUX");
	sleep(5);
	write_to("/sys/class/android_usb/android0/enable", "1");
  }

  return 0;
}