#ifndef _ATFWD_H_
#define _ATFWD_H_
#include <stdbool.h>

#define VERSION "0.0.0"

/* Devices */
#define DPL "/dev/dpl_ctrl"
#define RMNET_CTL "/dev/rmnet_ctrl"
#define SMD_CNTL "/dev/smdcntl8"
#define IPC_ROUTER 27 // AF_IB
#define IPC_ROUTER_ADDR 2 // Kernel IPC driver address
#define IPC_ROUTER_ADDRTYPE 2 // From the decoded packets, this should be 2, not 1 like for DPM
#define IPC_HEXAGON_NODE 0x3
#define IPC_HEXAGON_PORT  0x1b
#define RMNET_CONN_ID 8
#define IPC_IOCTL_MAGIC 0xc3
#define IOCTL_BIND_TOIPC _IOR(IPC_IOCTL_MAGIC, 4, uint32_t)
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


/* Playing around */
struct portmapper_port_map_arr {
	char *port_name;
	struct peripheral_ep_info epinfo;
};

struct portmapper_open_request {
	uint8_t is_valid_ctl_list;
	uint32_t ctl_list_length;
	struct portmapper_port_map_arr hw_port_map[2];

	uint8_t is_valid_hw_list;
	uint32_t hw_list_length;
	struct ep_info hw_epinfo;

	uint8_t is_valid_sw_list;
	uint32_t sw_list_length;
	struct portmapper_port_map_arr sw_port_map[2];
};


#define DIAG_SERVICE 4097
typedef uint8_t ctl_state_t;

static const struct {
	unsigned int command_id;
	const char *cmd;
} at_commands[] = {
	{0, "+QNAND", },
	{1, "+QPRTPARA", },
	{2, "+QWSERVER", },
	{3, "+QWTOCLIEN", },
	{4, "+QWTOCLI", },
	{5, "+QWPARAM", },
	{6, "+QDATAFWD", },
	{7, "+QFTCMD", },
	{8, "+QBTPWR", },
	{9, "+QBTLEADDR", },
	{10, "+QBTNAME", },
	{11, "+QBTGATREG", },
	{12, "+QBTGATSS", },
	{13, "+QBTGATSC", },
	{14, "+QBTGATSD", },
	{15, "+QBTGATSIND", },
	{16, "+QBTGATSNOD", },
	{17, "+QBTGATRRSP", },
	{18, "+QBTGATWRSP", },
	{19, "+QBTGATSENLE", },
	{20, "+QBTGATADV", },
	{21, "+QBTGATDISC", },
	{22, "+QBTGATPER", },
	{23, "+QBTGATDBALC", },
	{24, "+QBTGATDBDEALC", },
	{25, "+QBTGATSA", },
	{26, "+QBTGATDA", },
	{27, "+QFCT", },
	{28, "+QFCTTX", },
	{29, "+QFCTRX", },
	{30, "+QBTSPPACT", },
	{31, "+QBTSPPDIC", },
	{32, "+QBTSPPWRS", },
	{33, "+QBTSCAN", },
	{34, "+QBTAVACT", },
	{35, "+QBTAVREG", },
	{36, "+QBTAVCON", },
	{37, "+QBTHFGCON", },
	{38, "+QIIC", },
	{39, "+QAUDLOOP", },
	{40, "+QDAI", },
	{41, "+QSIDET", },
	{42, "+QAUDMOD", },
	{43, "+QEEC", },
	{44, "+QMIC", },
	{45, "+QRXGAIN", },
	{46, "+QAUDRD", },
	{47, "+QAUDPLAY", },
	{48, "+QAUDSTOP", },
	{49, "+QPSND", },
	{50, "+QTTS", },
	{51, "+QTTSETUP", },
	{52, "+QLTONE", },
	{53, "+QLDTMF", },
	{54, "+QAUDCFG", },
	{55, "+QTONEDET", },
	{56, "+QWTTS", },
	{57, "+QPCMV", },
	{58, "+QTXIIR", },
	{59, "+QRXIIR", },
	{60, "+QPOWD", },
	{61, "+QSCLK", },
	{62, "+QCFG", },
	{63, "+QADBKEY", },
	{64, "+QADC", },
	{65, "+QADCTEMP", },
	{66, "+QGPSCFG", },
	{67, "+QODM", },
	{68, "+QFUMO", },
	{69, "+QFUMOCFG", },
	{70, "+QPRINT", },
	{71, "+QSDMOUNT", },
	{72, "+QFASTBOOT", },
	{73, "+QPSM", },
	{74, "+QPSMCFG", },
	{75, "+QLINUXCPU", },
	{76, "+QVERSION", },
	{77, "+QSUBSYSVER", },
	{78, "+QTEMPDBG", },
	{79, "+QTEMP", },
	{80, "+QTEMPDBGLVL", },
	{81, "+QDIAGPORT", },
	{82, "+QLPMCFG", },
	{83, "+QSGMIICFG", },
	{84, "+QWWAN", },
	{85, "+QLWWANUP", },
	{86, "+QLWWANDOWN", },
	{87, "+QLWWANSTATUS", },
	{88, "+QLWWANURCCFG", },
	{89, "+QLWWANCID", },
	{90, "+QLPING", },
	{91, "+QWIFI", },
	{92, "+QWSSID", },
	{93, "+QWSSIDHEX", },
	{94, "+QWAUTH", },
	{95, "+QWMOCH", },
	{96, "+QWISO", },
	{97, "+QWBCAST", },
	{98, "+QWCLICNT", },
	{99, "+QWCLIP", },
	{100, "+QWCLILST", },
	{101, "+QWSTAINFO", },
	{102, "+QWCLIRM", },
	{103, "+QWSETMAC", },
	{104, "+QWRSTD", },
	{105, "+QWIFICFG", },
	{106, "+QAPRDYIND", },
	{107, "+QFOTADL", },
	{108, "+HEREWEGO", },
};

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
enum {
	STATE_INIT = 0,
	STATE_GOT_CID = 1,
	STATE_RESET = 2,
};

struct qmi_device {
	int32_t fd;
	struct sockaddr_msm_ipc socket;
	uint8_t service;
	uint8_t transaction_id;
	ctl_state_t ctl_state;
	uint8_t buf[QMI_DEFAULT_BUF_SIZE];
	uint32_t handle;
	uint8_t cid;
	uint8_t ctl_num_cids;
	uint16_t cur_qmux_length;
	uint16_t qmux_progress;
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


struct atcmd_reg_request {
	// QMI Header
	uint8_t ctlid; // 0x00
	uint16_t transaction_id; // incremental counter for each request
	uint16_t msgid; // 0x20 0x00
    uint16_t length; // Sizeof rest of pack?
	
	// the request packet itself
	// Obviously unfinished :)
	uint8_t dummy1; // always 0x00 0x01
	uint8_t var1; //0x0a - 0x0f || 0x11 - 0x13
//	uint8_t dummy4; // always 0x00 0x01
	uint16_t dummy2; // always 0x00 0x01
	uint8_t var2; // 0x07 - 0x0e
	uint16_t dummy3; // always 0x00 0x01
	uint8_t var3;
	char *command; // The command itself (+CFUN, +QDAI...)
	uint8_t fillzero[47]; // Fills 47 bytes with zeroes on _every_ command...
	
} __attribute__((packed));

#endif