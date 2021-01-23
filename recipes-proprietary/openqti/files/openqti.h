#ifndef _OPENQTI_H_
#define _OPENQTI_H_

#define VERSION "v0.0.0"

#define SMD_CNTL "/dev/smdcntl8"
#define RMNET_CTL "/dev/rmnet_ctrl"
#define USB_TTY "/dev/ttyGS0"
#define SMD_TTY "/dev/smd8"
#define DPL "/dev/dpl_ctrl"
#define SMEMLOG "/dev/smem_log"
#define DIAGDEV "/dev/diag"
#define USB_TETHERED_SMD_CH "DATA40_CNTL"
#define IPPROTO_IP 0

#define IPC_ROUTER 27 // AF_IB
#define IPC_ROUTER_ADDR 2 // Kernel IPC driver address
#define IPC_ROUTER_ADDRTYPE 1 // As specified in the kernel
#define IPC_HEXAGON_NODE 0x3 //DST:<0x3:0x1c> 
#define IPC_HEXAGON_PORT 0x1c
#define IPC_HEXAGON_RESPONSE_PORT 0x1d
#define RMNET_CONN_ID 8
#define IPC_IOCTL_MAGIC 0xc3
#define QMI_DPM_PORT_NAME_MAX_V01 32
#define QMI_DPM_OPEN_PORT_REQ_V01 0x0020
#define QTI_QMI_MSG_TIMEOUT_VALUE           90000


#define MAX_PACKET_SIZE 2048 // rmnet max packet size
#define TRACED_DIAG_SZ 100000
#define MAX_QTI_PKT_SIZE 8192


#define QTI_IOCTL_MAGIC	'r'
#define GET_LINE_STATE	_IOR(QTI_IOCTL_MAGIC, 2, int)
#define EP_LOOKUP _IOR(QTI_IOCTL_MAGIC, 3, struct ep_info)
#define MODEM_OFFLINE _IO(QTI_IOCTL_MAGIC, 4)
#define MODEM_ONLINE _IO(QTI_IOCTL_MAGIC, 5)

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
#define IPC_ROUTER_IOCTL_LOOKUP_SERVER _IOWR(IPC_IOCTL_MAGIC, 2, struct sockaddr_msm_ipc)


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

#endif