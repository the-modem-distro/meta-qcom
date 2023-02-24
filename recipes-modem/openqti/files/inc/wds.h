// SPDX-License-Identifier: MIT
#ifndef __WDS_H__
#define __WDS_H__

#include "openqti.h"
#include "qmi.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Wireless data service
 *	https://gitlab.freedesktop.org/mobile-broadband/libqmi/
 *
 *
 */

#define DEFAULT_APN_NAME "internet"
enum rmnet_ioctl_cmds_e {
	RMNET_IOCTL_SET_LLP_ETHERNET = 0x000089F1, /* Set Ethernet protocol  */
	RMNET_IOCTL_SET_LLP_IP       = 0x000089F2, /* Set RAWIP protocol     */
	RMNET_IOCTL_GET_LLP          = 0x000089F3, /* Get link protocol      */
	RMNET_IOCTL_SET_QOS_ENABLE   = 0x000089F4, /* Set QoS header enabled */
	RMNET_IOCTL_SET_QOS_DISABLE  = 0x000089F5, /* Set QoS header disabled*/
	RMNET_IOCTL_GET_QOS          = 0x000089F6, /* Get QoS header state   */
	RMNET_IOCTL_GET_OPMODE       = 0x000089F7, /* Get operation mode     */
	RMNET_IOCTL_OPEN             = 0x000089F8, /* Open transport port    */
	RMNET_IOCTL_CLOSE            = 0x000089F9, /* Close transport port   */
	RMNET_IOCTL_FLOW_ENABLE      = 0x000089FA, /* Flow enable            */
	RMNET_IOCTL_FLOW_DISABLE     = 0x000089FB, /* Flow disable           */
	RMNET_IOCTL_FLOW_SET_HNDL    = 0x000089FC, /* Set flow handle        */
	RMNET_IOCTL_EXTENDED         = 0x000089FD, /* Extended IOCTLs        */
	RMNET_IOCTL_MAX
};

enum {
  WDS_APN_TYPE_UNKNOWN = 0,
  WDS_APN_TYPE_INTERNET = 1,
  WDS_APN_TYPE_IMS = 2,
  WDS_APN_TYPE_VIRTUAL_SIM = 3,
};

enum {
  WDS_RESET = 0x0000,
  WDS_SET_EVENT_REPORT = 0x0001,
  WDS_EVENT_REPORT = 0x0001,
  WDS_ABORT = 0x0002,
  WDS_INDICATION_REGISTER = 0x0003,
  WDS_GET_SUPPORTED_MESSAGES = 0x001E,
  WDS_START_NETWORK = 0x0020,
  WDS_STOP_NETWORK = 0x0021,
  WDS_GET_PACKET_SERVICE_STATUS = 0x0022,
  WDS_PACKET_SERVICE_STATUS = 0x0022,
  WDS_GET_CHANNEL_RATES = 0x0023,
  WDS_GET_PACKET_STATISTICS = 0x0024,
  WDS_GO_DORMANT = 0x0025,
  WDS_GO_ACTIVE = 0x0026,
  WDS_CREATE_PROFILE = 0x0027,
  WDS_MODIFY_PROFILE = 0x0028,
  WDS_DELETE_PROFILE = 0x0029,
  WDS_GET_PROFILE_LIST = 0x002A,
  WDS_GET_PROFILE_SETTINGS = 0x002B,
  WDS_GET_DEFAULT_SETTINGS = 0x002C,
  WDS_GET_CURRENT_SETTINGS = 0x002D,
  WDS_GET_DORMANCY_STATUS = 0x0030,
  WDS_GET_AUTOCONNECT_SETTINGS = 0x0034,
  WDS_GET_DATA_BEARER_TECHNOLOGY = 0x0037,
  WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY = 0x0044,
  WDS_GET_DEFAULT_PROFILE_NUMBER = 0x0049,
  WDS_SET_DEFAULT_PROFILE_NUMBER = 0x004A,
  WDS_SET_IP_FAMILY = 0x004D,
  WDS_SET_AUTOCONNECT_SETTINGS = 0x0051,
  WDS_GET_PDN_THROTTLE_INFO = 0x006C,
  WDS_GET_LTE_ATTACH_PARAMETERS = 0x0085,
  WDS_BIND_DATA_PORT = 0x0089,
  WDS_EXTENDED_IP_CONFIG = 0x008C,
  WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER = 0x0092,
  WDS_SET_LTE_ATTACH_PDN_LIST = 0x0093,
  WDS_GET_LTE_ATTACH_PDN_LIST = 0x0094,
  WDS_BIND_MUX_DATA_PORT = 0x00A2,
  WDS_CONFIGURE_PROFILE_EVENT_LIST = 0x00A7,
  WDS_PROFILE_CHANGED = 0x00A8,
  WDS_SWI_CREATE_PROFILE_INDEXED = 0x5558,
};

enum {
   WDS_EXTENDED_ERROR_CODE = 0xE0,
  WDS_PROFILE_IDENTIFIER = 0x01,
  WDS_PROFILE_NAME = 0x10,
  WDS_PDP_TYPE = 0x11,
  WDS_PDP_HEADER_COMPRESSION_TYPE = 0x12,
  WDS_PDP_DATA_COMPRESSION_TYPE = 0x13,
  WDS_APN_NAME = 0x14,
  WDS_PRIMARY_IPV4_DNS_ADDRESS = 0x15,
  WDS_SECONDARY_IPV4_DNS_ADDRESS = 0x16,
  WDS_UMTS_REQUESTED_QOS = 0x17,
  WDS_UMTS_MINIMUM_QOS = 0x18,
  WDS_GPRS_REQUESTED_QOS = 0x19,
  WDS_GPRS_MINIMUM_QOS = 0x1A,
  WDS_USERNAME = 0x1B,
  WDS_PASSWORD = 0x1C,
  WDS_AUTHENTICATION = 0x1D,
  WDS_IPV4_ADDRESS_PREFERENCE = 0x1E,
  WDS_PCSCF_ADDRESS_USING_PCO = 0x1F,
  WDS_PCSCF_ADDRESS_USING_DHCP = 0x21,
  WDS_IMCN_FLAG = 0x22,
  WDS_PDP_CONTEXT_NUMBER = 0x25,
  WDS_PDP_CONTEXT_SECONDARY_FLAG = 0x26,
  WDS_PDP_CONTEXT_PRIMARY_ID = 0x27,
  WDS_IPV6_ADDRESS_PREFERENCE = 0x28,
  WDS_UMTS_REQUESTED_QOS_WITH_SIGNALING_INDICATION_FLAG = 0x29,
  WDS_UMTS_MINIMUM_QOS_WITH_SIGNALING_INDICATION_FLAG = 0x2A,
  WDS_IPV6_PRIMARY_DNS_ADDRESS_PREFERENCE = 0x2B,
  WDS_IPV6_SECONDARY_DNS_ADDRESS_PREFERENCE = 0x2C,
  WDS_LTE_QOS_PARAMETERS = 0x2E,
  WDS_APN_DISABLED_FLAG = 0x2F,
  WDS_ROAMING_DISALLOWED_FLAG = 0x3E,
  WDS_APN_TYPE_MASK = 0xDD,
};

static const struct {
  uint16_t id;
  const char *cmd;
} wds_svc_commands[] = {
    {WDS_RESET, "Reset"},
    {WDS_SET_EVENT_REPORT, "Set Event Report"},
    {WDS_EVENT_REPORT, "Event Report"},
    {WDS_ABORT, "Abort"},
    {WDS_INDICATION_REGISTER, "Indication Register"},
    {WDS_GET_SUPPORTED_MESSAGES, "Get Supported Messages"},
    {WDS_START_NETWORK, "Start Network"},
    {WDS_STOP_NETWORK, "Stop Network"},
    {WDS_GET_PACKET_SERVICE_STATUS, "Get Packet Service Status"},
    {WDS_PACKET_SERVICE_STATUS, "Packet Service Status"},
    {WDS_GET_CHANNEL_RATES, "Get Channel Rates"},
    {WDS_GET_PACKET_STATISTICS, "Get Packet Statistics"},
    {WDS_GO_DORMANT, "Go Dormant"},
    {WDS_GO_ACTIVE, "Go Active"},
    {WDS_CREATE_PROFILE, "Create Profile"},
    {WDS_MODIFY_PROFILE, "Modify Profile"},
    {WDS_DELETE_PROFILE, "Delete Profile"},
    {WDS_GET_PROFILE_LIST, "Get Profile List"},
    {WDS_GET_PROFILE_SETTINGS, "Get Profile Settings"},
    {WDS_GET_DEFAULT_SETTINGS, "Get Default Settings"},
    {WDS_GET_CURRENT_SETTINGS, "Get Current Settings"},
    {WDS_GET_DORMANCY_STATUS, "Get Dormancy Status"},
    {WDS_GET_AUTOCONNECT_SETTINGS, "Get Autoconnect Settings"},
    {WDS_GET_DATA_BEARER_TECHNOLOGY, "Get Data Bearer Technology"},
    {WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY,
     "Get Current Data Bearer Technology"},
    {WDS_GET_DEFAULT_PROFILE_NUMBER, "Get Default Profile Number"},
    {WDS_SET_DEFAULT_PROFILE_NUMBER, "Set Default Profile Number"},
    {WDS_SET_IP_FAMILY, "Set IP Family"},
    {WDS_SET_AUTOCONNECT_SETTINGS, "Set Autoconnect Settings"},
    {WDS_GET_PDN_THROTTLE_INFO, "Get PDN Throttle Info"},
    {WDS_GET_LTE_ATTACH_PARAMETERS, "Get LTE Attach Parameters"},
    {WDS_BIND_DATA_PORT, "Bind Data Port"},
    {WDS_EXTENDED_IP_CONFIG, "Extended Ip Config"},
    {WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER, "Get Max LTE Attach PDN Number"},
    {WDS_SET_LTE_ATTACH_PDN_LIST, "Set LTE Attach PDN List"},
    {WDS_SET_LTE_ATTACH_PDN_LIST, "Set LTE Attach PDN List"},
    {WDS_GET_LTE_ATTACH_PDN_LIST, "Get LTE Attach PDN List"},
    {WDS_BIND_MUX_DATA_PORT, "Bind Mux Data Port"},
    {WDS_CONFIGURE_PROFILE_EVENT_LIST, "Configure Profile Event List"},
    {WDS_PROFILE_CHANGED, "Profile Changed"},
    {WDS_SWI_CREATE_PROFILE_INDEXED, "Swi Create Profile Indexed"},
};

enum {
  WDS_CALL_TYPE_LAPTOP = 0x00,
  WDS_CALL_TYPE_EMBEDDED = 0x01,
};

struct apn_config {
  uint8_t type; // 0x14
  uint16_t len; // sizeof
  char apn[0];  // Null to get default, name
} __attribute__((packed));

struct apn_type {
  uint8_t type;   // 0x38
  uint16_t len;   // 0x04 (uint32)
  uint32_t value; // See APN_TYPE enums
} __attribute__((packed));

struct ip_family_preference {
  uint8_t type;  // 0x19
  uint16_t len;  // 0x01 (uint8)
  uint8_t value; // 4 || 6 || 8 -> IPv4 || IPv6 || Not specified
} __attribute__((packed));

struct profile_index {
  uint8_t type;  // 0x31
  uint16_t len;  // 0x01 (uint8)
  uint8_t value; // I think this is the PDP context ID
} __attribute__((packed));

struct autoconnect_config {
  uint8_t type;  // 0x33
  uint16_t len;  // 0x01 (uint8)
  uint8_t value; // 0 OFF || 1 ON
} __attribute__((packed));

struct call_type {
  uint8_t type;  // 0x35
  uint16_t len;  // 0x01 (uint8)
  uint8_t value; // 0 LAPTOP || 1 EMBEDDED
} __attribute__((packed));

struct block_in_roaming {
  uint8_t type;  // 0x39
  uint16_t len;  // 0x01 (uint8)
  uint8_t value; // 0 OFF || 1 Don't connect if roaming
} __attribute__((packed));

struct ep_id {
  uint8_t type;          // 0x10
  uint16_t len;          // 0x08
  uint32_t ep_type;      // peripheral_ep_type in ipc.h
  uint32_t interface_id; // ??
} __attribute__((packed));

struct mux_id {
  uint8_t type;   // 0x11
  uint16_t len;   // 0x01
  uint8_t mux_id; // from dpm
} __attribute__((packed));

struct wds_bind_mux_data_port_request {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
  struct ep_id peripheral_id;
  struct mux_id mux;
} __attribute__((packed));

struct wds_auth_pref {
  uint8_t type;   // 0x11
  uint16_t len;   // 0x01
  uint8_t preference; // Bit 0: PAP 0||1 ; Bit 1: CHAP 0||1
} __attribute__((packed));

struct wds_auth_username { // 0x17
  uint8_t type;
  uint16_t len;
  char username[0];
} __attribute__((packed));

struct wds_auth_password { // 0x18
  uint8_t type;
  uint16_t len;
  char password[0];
} __attribute__((packed));

struct wds_start_network {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
  struct apn_config apn;
  struct apn_type apn_type;
  struct ip_family_preference ip_family;
//  struct profile_index profile;
  struct call_type call_type;
//  struct autoconnect_config autoconnect;
  struct block_in_roaming roaming_lock;
} __attribute__((packed));

struct wds_start_network_autoconnect {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
  struct autoconnect_config autoconnect;
} __attribute__((packed));

void reset_wds_runtime();
uint8_t is_wds_initialized();
int wds_attempt_to_connect();
void *init_internal_networking();
const char *get_wds_command(uint16_t msgid);

int handle_incoming_wds_message(uint8_t *buf, size_t buf_len);
#endif
