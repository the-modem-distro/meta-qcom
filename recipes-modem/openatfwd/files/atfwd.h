#ifndef _ATFWD_H_
#define _ATFWD_H_
#include <stdbool.h>

#define VERSION "0.0.2"

/* Devices */
#define DPL "/dev/dpl_ctrl"
#define RMNET_CTL "/dev/rmnet_ctrl"
#define SMD_CNTL "/dev/smdcntl8"
#define SND_CTL  "/dev/snd/controlC0"
#define IPC_ROUTER 27 // AF_IB
#define IPC_ROUTER_ADDR 2 // Kernel IPC driver address
#define IPC_ROUTER_ADDRTYPE 1 // As specified in the kernel
#define IPC_HEXAGON_NODE 0x3 //DST:<0x3:0x1c> 
#define IPC_HEXAGON_PORT 0x1c
#define RMNET_CONN_ID 8
#define IPC_IOCTL_MAGIC 0xc3

#define MAX_PACKET_SIZE 2048 // rmnet max packet size

#define QTI_IOCTL_MAGIC	'r'
#define GET_LINE_STATE	_IOR(QTI_IOCTL_MAGIC, 2, int)
#define EP_LOOKUP _IOR(QTI_IOCTL_MAGIC, 3, struct ep_info)


#define QMI_SERVICE_CTL         0x00
#define QMI_DEFAULT_BUF_SIZE    0x1000

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

#define DIAG_SERVICE 4097
typedef uint8_t ctl_state_t;

// From lookup.c, useful to know which service you're at
static const struct {
	unsigned int service;
	unsigned int ifilter;
	const char *name;
} common_names[] = {
	{ 0, 0, "Control service" },
	{ 1, 0, "Wireless Data Service" },
	{ 2, 0, "Device Management Service" },
	{ 3, 0, "Network Access Service" },
	{ 4, 0, "Quality Of Service service" },
	{ 5, 0, "Wireless Messaging Service" },
	{ 6, 0, "Position Determination Service" },
	{ 7, 0, "Authentication service" },
	{ 8, 0, "AT service" },
	{ 9, 0, "Voice service" },
	{ 10, 0, "Card Application Toolkit service (v2)" },
	{ 11, 0, "User Identity Module service" },
	{ 12, 0, "Phonebook Management service" },
	{ 13, 0, "QCHAT service" },
	{ 14, 0, "Remote file system service" },
	{ 15, 0, "Test service" },
	{ 16, 0, "Location service (~ PDS v2)" },
	{ 17, 0, "Specific absorption rate service" },
	{ 18, 0, "IMS settings service" },
	{ 19, 0, "Analog to digital converter driver service" },
	{ 20, 0, "Core sound driver service" },
	{ 21, 0, "Modem embedded file system service" },
	{ 22, 0, "Time service" },
	{ 23, 0, "Thermal sensors service" },
	{ 24, 0, "Thermal mitigation device service" },
	{ 25, 0, "Service access proxy service" },
	{ 26, 0, "Wireless data administrative service" },
	{ 27, 0, "TSYNC control service" },
	{ 28, 0, "Remote file system access service" },
	{ 29, 0, "Circuit switched videotelephony service" },
	{ 30, 0, "Qualcomm mobile access point service" },
	{ 31, 0, "IMS presence service" },
	{ 32, 0, "IMS videotelephony service" },
	{ 33, 0, "IMS application service" },
	{ 34, 0, "Coexistence service" },
	{ 36, 0, "Persistent device configuration service" },
	{ 38, 0, "Simultaneous transmit service" },
	{ 39, 0, "Bearer independent transport service" },
	{ 40, 0, "IMS RTP service" },
	{ 41, 0, "RF radiated performance enhancement service" },
	{ 42, 0, "Data system determination service" },
	{ 43, 0, "Subsystem control service" },
	{ 47, 0, "Data Port Mapper service" },
	{ 49, 0, "IPA control service" },
	{ 50, 0, "-- UIMRMT QMI Service "},
	{ 51, 0, "CoreSight remote tracing service" },
	{ 52, 0, "Dynamic Heap Memory Sharing" },
	{ 55, 0, "-- QMI-SLIM service"},
	{ 56,0, "-- LOWI" },
	{ 57, 0, "-- WLPS" }, 
	{ 64, 0, "Service registry locator service" },
	{ 66, 0, "Service registry notification service" },
	{ 69, 0, "ATH10k WLAN firmware service" },
	{ 71, 0, "UIMHTTP" },
	{ 77, 0, "-- RCS " },
	{ 224, 0, "Card Application Toolkit service (v1)" },
	{ 225, 0, "Remote Management Service" },
	{ 226, 0, "Open Mobile Alliance device management service" },
	{ 235, 0, "-- MODEM_SVC"}, 
	{ 256, 0, "-- Sensor services"},
	{ 312, 0, "QBT1000 Ultrasonic Fingerprint Sensor service" },
	{ 769, 0, "SLIMbus control service" },
	{ 771, 0, "Peripheral Access Control Manager service" },
	{ 4096, 0, "TFTP" },
	{ DIAG_SERVICE, 0, "DIAG service" },
};

/* QMI Device handler proto
 *
 *	The idea is to be able to have multiple handlers for
 *  different services subscribed, each with their transaction
 *  IDs to manage or sniff different services.
 *  First one to check is 47 or 0x2F, which is the DPM, to play
 *  with modem init and request DATA40_CNTL properly
 */
struct qmi_device {
	int32_t fd;
	struct sockaddr_msm_ipc socket;
	uint8_t service;
	uint8_t transaction_id;
	ctl_state_t ctl_state;
	uint8_t buf[QMI_DEFAULT_BUF_SIZE];
	uint32_t handle;
};


/* From qmi_hdrs.h at qmi-dialer */

//qmux header
struct qmux_header{
    uint16_t length;
    uint8_t control_flags;
    uint8_t service_type;
    uint8_t client_id;
} __attribute__((packed));

//The two different types of QMI headers I have seen. According to the
//specification, the size of the header is implementation-dependant (depending
//on service)
struct qmi_header_ctl{
    uint8_t control_flags;
    uint8_t transaction_id;
    uint16_t message_id;
    uint16_t length;
} __attribute__((packed));

struct qmi_header_gen{
    uint8_t control_flags;
    uint16_t transaction_id;
    uint16_t message_id;
    uint16_t length;
} __attribute__((packed));

struct qmi_tlv{
    uint8_t type;
    uint16_t length;
} __attribute__((packed));

typedef struct qmux_header qmux_hdr_t;
typedef struct qmi_header_ctl qmi_hdr_ctl_t;
typedef struct qmi_header_gen qmi_hdr_gen_t;
typedef struct qmi_tlv qmi_tlv_t;

#define QMI_DEFAULT_BUF_SIZE    0x1000
#define QMI_CTL_RELEASE_CID     0x0023

//Control flags
#define QMI_CTL_FLAGS_RESP      0x3
#define QMI_CTL_FLAGS_IND       0x4

//Variables
#define QMI_RESULT_SUCCESS      0x0000
#define QMI_RESULT_FAILURE      0x0001

//Return values for message passing functions
#define QMI_MSG_SUCCESS         0x0
#define QMI_MSG_FAILURE         0x1
#define QMI_MSG_IGNORE          0x2

//Different sates for each service type
enum{
    CTL_NOT_SYNCED = 0,
    CTL_SYNCED,
};


//CTL message types
#define QMI_CTL_GET_CID         0x0022
#define QMI_CTL_RELEASE_CID     0x0023
#define QMI_CTL_SET_DATA_FORMAT	0x0026
#define QMI_CTL_SYNC            0x0027

//CTL TLVs
#define QMI_CTL_TLV_ALLOC_INFO  0x01

//Set data format
#define	QMI_CTL_TLV_DATA_FORMAT	0x01
#define QMI_CTL_TLV_DATA_PROTO	0x10


#endif