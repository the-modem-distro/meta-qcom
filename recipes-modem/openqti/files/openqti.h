#ifndef _OPENQTI_H_
#define _OPENQTI_H_

#define VERSION "0.0.2"

/* Devices */
#define DPL "/dev/dpl_ctrl"
#define RMNET_CTL "/dev/rmnet_ctrl"
#define SMD_CNTL "/dev/smdcntl8"
#define SND_CTL  "/dev/snd/controlC0"

#define PCM_DEV_VOCS "/dev/snd/pcmC0D2" // Normal Voice calls use CS - Circuit Switch, device #2
#define PCM_DEV_VOLTE "/dev/snd/pcmC0D4" // VoLTE uses PCM device #4

#define IPC_ROUTER 27 // AF_IB
#define IPC_ROUTER_ADDR 2 // Kernel IPC driver address
#define IPC_ROUTER_ADDRTYPE 1 // As specified in the kernel
#define IPC_HEXAGON_NODE 0x3 //DST:<0x3:0x1c> 
#define IPC_HEXAGON_PORT 0x1c
#define RMNET_CONN_ID 8
#define IPC_IOCTL_MAGIC 0xc3
#define IOCTL_BIND_TOIPC _IOR(IPC_IOCTL_MAGIC, 4, uint32_t)

#define MAX_PACKET_SIZE 2048 // rmnet max packet size

#define QTI_IOCTL_MAGIC	'r'
#define GET_LINE_STATE	_IOR(QTI_IOCTL_MAGIC, 2, int)
#define EP_LOOKUP _IOR(QTI_IOCTL_MAGIC, 3, struct ep_info)
#define MODEM_OFFLINE _IO(QTI_IOCTL_MAGIC, 4)
#define MODEM_ONLINE _IO(QTI_IOCTL_MAGIC, 5)
// for handle_pkt
#define FROM_DSP 0
#define FROM_HOST 1

struct msm_ipc_port_addr {
	uint32_t node_id;
	uint32_t port_id;
};

struct msm_ipc_port_name {
	uint32_t service;
	uint32_t instance;
};

struct msm_ipc_addr {
	unsigned char  addrtype;
	union {
		struct msm_ipc_port_addr port_addr;
		struct msm_ipc_port_name port_name;
	} addr;
};
struct sockaddr_msm_ipc {
	unsigned short family;
	struct msm_ipc_addr address;
	unsigned char reserved;
};

enum peripheral_ep_type {
	DATA_EP_TYPE_RESERVED	= 0x0,
	DATA_EP_TYPE_HSIC	= 0x1,
	DATA_EP_TYPE_HSUSB	= 0x2,
	DATA_EP_TYPE_PCIE	= 0x3,
	DATA_EP_TYPE_EMBEDDED	= 0x4,
	DATA_EP_TYPE_BAM_DMUX	= 0x5,
};

struct peripheral_ep_info {
	enum peripheral_ep_type ep_type;
	unsigned long int peripheral_iface_id;
};

struct ipa_ep_pair {
	unsigned long int cons_pipe_num;
	unsigned long int prod_pipe_num;
};

struct ep_info {
	struct peripheral_ep_info	ph_ep_info;
	struct ipa_ep_pair ipa_ep_pair;
};

struct mixer *mixer;
struct pcm *pcm_tx;
struct pcm *pcm_rx;
bool is_call_active = false;

static const struct {
	const char *path;
	const char *value;
} sysfs_value_pairs[] = {
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/mode", "0" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/sync", "0" },
	{ "/sys/devices/soc:sound/pcm_mode_select", "0" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/frame", "2" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/data", "1" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/rate", "256000" },
	{ "/sys/devices/soc:sound/quec_auxpcm_rate", "8000" },
};
#endif