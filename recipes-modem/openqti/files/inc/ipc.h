/* SPDX-License-Identifier: MIT */

#ifndef _IPC_H_
#define _IPC_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "qmi.h"

#define IPC_ROUTER 27     // AF_IB
#define IPC_ROUTER_ADDR 2 // Kernel IPC driver address

/* When we get to mainline we'll stop using MSM IPC...*/
#define AF_QRTR 42

// As specified in the kernel
#define IPC_ROUTER_DPM_ADDRTYPE 1
// From the decoded packets, this should be 2, not 1 like for DPM
#define IPC_ROUTER_AT_ADDRTYPE 2

#define IPC_HEXAGON_NODE 0x3
#define IPC_HEXAGON_DPM_PORT 0x1c
#define IPC_HEXAGON_ATFWD_PORT 0x1b

#define SMDCTLPORTNAME "DATA40_CNTL"
#define LOCAL_SMD_CTL_PORT_NAME "DATA5_CNTL"

#define RMNET_CONN_ID 8
#define IPC_IOCTL_MAGIC 0xc3
#define IOCTL_BIND_TOIPC _IOR(IPC_IOCTL_MAGIC, 4, uint32_t)

#define QTI_IOCTL_MAGIC 'r'
#define GET_LINE_STATE _IOR(QTI_IOCTL_MAGIC, 2, int)

#define IPC_ROUTER_IOCTL_LOOKUP_SERVER                                         \
  _IOC(_IOC_READ | _IOC_WRITE, IPC_IOCTL_MAGIC, 0x2,                           \
       sizeof(struct sockaddr_msm_ipc))
#define EP_LOOKUP _IOR(QTI_IOCTL_MAGIC, 3, struct ep_info)
#define MODEM_OFFLINE _IO(QTI_IOCTL_MAGIC, 4)
#define MODEM_ONLINE _IO(QTI_IOCTL_MAGIC, 5)

#define MAX_PACKET_SIZE 6144 // rmnet max packet size

// IPC Port security rules
#define IOCTL_RULES _IOR(0xC3, 5, struct irsc_rule)
#define IRSC_INSTANCE_ALL 4294967295

// Client release command request
#define CLIENT_REGISTER_REQ 0x0022
#define CLIENT_RELEASE_REQ 0x0023
#define CLIENT_REG_TIMEOUT 2400000

/* ModemManager will connect first  to the WDA service, while
  oFono's first connection attempt is to the DMS service.
  We use that to identify our host app  
*/
#define HOST_USES_MODEMMANAGER 0x1a
#define HOST_USES_OFONO 0x02

struct irsc_rule {
  int rl_no;
  uint32_t service;
  uint32_t instance;
  unsigned dummy;
  gid_t group_id[0];
};

// QMI Service names
static const struct {
  uint8_t service;
  const char *name;
} qmi_services[] = {
    {0x00, "Control service"},
    {0x01, "Wireless Data Service"},
    {0x02, "Device Management Service"},
    {0x03, "Network Access Service"},
    {0x04, "Quality Of Service service"},
    {0x05, "Wireless Messaging Service"},
    {0x06, "Position Determination Service"},
    {0x07, "Authentication service"},
    {0x08, "AT service"},
    {0x09, "Voice service"},
    {0x0a, "Card Application Toolkit service (v2)"},
    {0x0b, "User Identity Module service"},
    {0x0c, "Phonebook Management service"},
    {0x0d, "QCHAT service"},
    {0x0e, "Remote file system service"},
    {0x0f, "Test service"},
    {0x10, "Location service (~ PDS v2)"},
    {0x11, "Specific absorption rate service"},
    {0x12, "IMS settings service"},
    {0x13, "Analog to digital converter driver service"},
    {0x14, "Core sound driver service"},
    {0x15, "Modem embedded file system service"},
    {0x16, "Time service"},
    {0x17, "Thermal sensors service"},
    {0x18, "Thermal mitigation device service"},
    {0x19, "Service access proxy service"},
    {0x1a, "Wireless data administrative service"},
    {0x1b, "TSYNC control service"},
    {0x1c, "Remote file system access service"},
    {0x1d, "Circuit switched videotelephony service"},
    {0x1e, "Qualcomm mobile access point service"},
    {0x1f, "IMS presence service"},
    {0x20, "IMS videotelephony service"},
    {0x21, "IMS application service"},
    {0x22, "Coexistence service"},
    {0x23, "UNKNOWN SERVICE"},
    {0x24, "Persistent device configuration service"},
    {0x25, "UNKNOWN SERVICE"},
    {0x26, "Simultaneous transmit service"},
    {0x27, "Bearer independent transport service"},
    {0x28, "IMS RTP service"},
    {0x29, "RF radiated performance enhancement service"},
    {0x2a, "Data system determination service"},
    {0x2b, "Subsystem control service"},
    {0x2c, "Modem external Filesystem service"},
    {0x2d, "MDM Scrubber service"},
    {0x2e, "UNKNOWN Service"},
    {0x2f, "Data Port Mapper service"},
    {0x30, "Data filter service"},
    {0x31, "IPA control service"},
    {0x32, "UIM remote service"},
    {0x33, "CoreSight remote tracing service"},
    {0x34, "Dynamic Heap Memory Sharing"},
    {0x35, "Subsystem control...request?"},
    {0x36, "Flow control management"},
    {0x37, "Sensor location interface"},
    {0x38, "lowi (location over wlan) service"},
    {0x3c, "Health monitor"},
    {0x3d, "Filesystem service"},
    {0x40, "Service registry locator service"},
    {0x42, "Service registry notification service"},
    {0x43, "Bandwidth limit manager"},
    {0x44, "OTT"},
    {0x45, "ATH10k WLAN firmware service"},
    {0x46, "LTE service"},
    {0x47, "UIM http service"},
    {0x48, "Non IP data service"},
    {0x4a, "Antenna switch service"},
    /* ... */
    {0xe0, "Card Application Toolkit service (v1)"},
    {0xe1, "Remote Management Service"},
    {0xe2, "Open Mobile Alliance device management service"},
};

struct msm_ipc_port_addr {
  uint32_t node_id;
  uint32_t port_id;
};

struct msm_ipc_port_name {
  uint32_t service;
  uint32_t instance;
};

struct msm_ipc_addr {
  unsigned char addrtype;
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
  DATA_EP_TYPE_RESERVED = 0x0,
  DATA_EP_TYPE_HSIC = 0x1,
  DATA_EP_TYPE_HSUSB = 0x2,
  DATA_EP_TYPE_PCIE = 0x3,
  DATA_EP_TYPE_EMBEDDED = 0x4,
  DATA_EP_TYPE_BAM_DMUX = 0x5,
} __attribute__((packed));

struct qmi_device {
  int32_t fd;
  struct sockaddr_msm_ipc socket;
  uint8_t service;
  uint8_t transaction_id;
  uint32_t handle;
  uint8_t cid;
};

struct hw_port {
  uint8_t id; // 0x10
  uint16_t len; // 0x15
  uint8_t valid_ctl_list;
  uint8_t hw_control_name_sz; // strlen(port_name)
  unsigned char port_name[11]; // DATA40_CNTL || DATA5_CNTL
  uint32_t ep_type; // 05 00 00 00
  uint32_t peripheral_id; // 08 00 00 00  || 00 00 00 00 ?? (smdcntl0)
}__attribute__((packed));

struct hw_port_sm {
  uint8_t id; // 0x10
  uint16_t len; // 0x15
  uint8_t valid_ctl_list;
  uint8_t hw_control_name_sz; // strlen(port_name)
  unsigned char port_name[10]; // DATA40_CNTL || DATA5_CNTL
  uint32_t ep_type; // 05 00 00 00
  uint32_t peripheral_id; // 08 00 00 00  || 00 00 00 00 ?? (smdcntl0)
}__attribute__((packed));

struct sw_port {
  uint8_t id; // 0x11
  uint16_t len; // 0x11 0x00
  uint8_t valid_ctl_list; // 0x01
  uint32_t ep_type; // 0x05 00 00 00
  uint32_t peripheral_id; // 0x08 00 00 00 || 0x00 00 00 00 ?? (smdcntl0)
  uint32_t consumer_pipe_num; // 00 00 00 00
  uint32_t prod_pipe_num; // 00 00 00 00
}__attribute__((packed));

struct portmapper_open_request_new {
  struct qmi_packet qmi;
  struct hw_port hw;
  struct sw_port sw;
} __attribute__((packed));

struct portmapper_open_request_shared {
  struct qmi_packet qmi;
  struct hw_port_sm hw;
  struct sw_port sw;
} __attribute__((packed));

// linux uapi
struct msm_ipc_server_info {
  uint32_t node_id;
  uint32_t port_id;
  uint32_t service;
  uint32_t instance;
};

struct server_lookup_args {
  struct msm_ipc_port_name port_name;
  int num_entries_in_array;
  int num_entries_found;
  uint32_t lookup_mask;
  struct msm_ipc_server_info srv_info[0];
};

struct service_pair {
  uint8_t service;
  uint8_t instance;
};
const char *get_qmi_service_name(uint8_t service);
int open_ipc_socket(struct qmi_device *qmisock, uint32_t node, uint32_t port,
                    uint32_t service, uint32_t instance,
                    unsigned char address_type);

bool is_server_active(uint32_t node, uint32_t port);

struct msm_ipc_server_info get_node_port(uint32_t service, uint32_t instance);
int find_services();
int init_port_mapper();
int init_port_mapper_internal();
int setup_ipc_security();
const char *get_service_name(uint8_t service_id);
#endif