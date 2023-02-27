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
    {0, "Control service"},
    {1, "Wireless Data Service"},
    {2, "Device Management Service"},
    {3, "Network Access Service"},
    {4, "Quality Of Service service"},
    {5, "Wireless Messaging Service"},
    {6, "Position Determination Service"},
    {7, "Authentication service"},
    {8, "AT service"},
    {9, "Voice service"},
    {10, "Card Application Toolkit service (v2)"},
    {11, "User Identity Module service"},
    {12, "Phonebook Management service"},
    {13, "QCHAT service"},
    {14, "Remote file system service"},
    {15, "Test service"},
    {16, "Location service (~ PDS v2)"},
    {17, "Specific absorption rate service"},
    {18, "IMS settings service"},
    {19, "Analog to digital converter driver service"},
    {20, "Core sound driver service"},
    {21, "Modem embedded file system service"},
    {22, "Time service"},
    {23, "Thermal sensors service"},
    {24, "Thermal mitigation device service"},
    {25, "Service access proxy service"},
    {26, "Wireless data administrative service"},
    {27, "TSYNC control service"},
    {28, "Remote file system access service"},
    {29, "Circuit switched videotelephony service"},
    {30, "Qualcomm mobile access point service"},
    {31, "IMS presence service"},
    {32, "IMS videotelephony service"},
    {33, "IMS application service"},
    {34, "Coexistence service"},
    {36, "Persistent device configuration service"},
    {38, "Simultaneous transmit service"},
    {39, "Bearer independent transport service"},
    {40, "IMS RTP service"},
    {41, "RF radiated performance enhancement service"},
    {42, "Data system determination service"},
    {43, "Subsystem control service"},
    {47, "Data Port Mapper service"},
    {49, "IPA control service"},
    {51, "CoreSight remote tracing service"},
    {52, "Dynamic Heap Memory Sharing"},
    {64, "Service registry locator service"},
    {66, "Service registry notification service"},
    {69, "ATH10k WLAN firmware service"},
    {224, "Card Application Toolkit service (v1)"},
    {225, "Remote Management Service"},
    {226, "Open Mobile Alliance device management service"},
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