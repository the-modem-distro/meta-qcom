// SPDX-License-Identifier: MIT
#ifndef __WDS_H__
#define __WDS_H__

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Wireless data service
 *
 *
 *
 */

#define DEFAULT_APN_NAME "internet"

enum {
    WDS_RESET = 0x0000,
    WDS_SET_EVENT_REPORT = 0x0001,
    WDS_ABORT = 0x0002,
    WDS_IND_REGISTER = 0x0003,
    WDS_GET_SUPPORTED_MSG = 0x001e,
    WDS_GET_PROFILE_LIST = 0x002a,
    WDS_GET_PROFILE_SETTINGS = 0x002b,
    WDS_START_NETWORK_IF = 0x0020,
    WDS_STOP_NETWORK_IF = 0x0021,
    WDS_BIND_MUX_DATA_PORT = 0x00a2,
    WDS_GET_AUTOCONNECT_SETTING = 0x0034,
    WDS_EVENT_IFACE_RF_CHANGED = 0x0008,
    WDS_EVENT_IFACE_DOS_ACK = 0x0009,
    WDS_GET_LTE_ATTACH_PARAMS = 0x0085,
    WDS_GET_LTE_ATTACH_PDN_LIST = 0x0094,

};

enum {
    WDS_APN_TYPE_UNKNOWN = 0,
    WDS_APN_TYPE_INTERNET = 1,
    WDS_APN_TYPE_IMS = 2,
    WDS_APN_TYPE_VIRTUAL_SIM = 3,
};

struct apn_config{
  uint8_t type; // 0x14
  uint16_t len; // sizeof
  char apn[8]; // Null to get default, name
} __attribute__((packed));

struct apn_type{
  uint8_t type; // 0x38
  uint16_t len; // 0x04 (uint32)
  uint32_t value; // See APN_TYPE enums
} __attribute__((packed));

struct ip_family_preference{
  uint8_t type; // 0x19
  uint16_t len; // 0x01 (uint8)
  uint8_t value; // 4 || 6 || 8 -> IPv4 || IPv6 || Not specified
} __attribute__((packed));

struct profile_index {
  uint8_t type; // 0x31
  uint16_t len; // 0x01 (uint8)
  uint8_t value; // I think this is the PDP context ID
} __attribute__((packed));

struct autoconnect_config{
  uint8_t type; // 0x33
  uint16_t len; // 0x01 (uint8)
  uint8_t value; // 0 OFF || 1 ON
} __attribute__((packed));

struct call_type{
  uint8_t type; // 0x35
  uint16_t len; // 0x01 (uint8)
  uint8_t value; // 0 LAPTOP || 1 EMBEDDED
} __attribute__((packed));


struct block_in_roaming{
  uint8_t type; // 0x39
  uint16_t len; // 0x01 (uint8)
  uint8_t value; // 0 OFF || 1 Don't connect if roaming
} __attribute__((packed));

struct ep_id{
  uint8_t type; // 0x10
  uint16_t len; // 0x08
  uint32_t ep_type; // peripheral_ep_type in ipc.h
  uint32_t interface_id; // ??
} __attribute__((packed));

struct mux_id{
  uint8_t type; // 0x11
  uint16_t len; // 0x01
  uint8_t mux_id; // from dpm
} __attribute__((packed));

struct wds_bind_mux_data_port_request {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
  struct ep_id peripheral_id;
  struct mux_id mux;
} __attribute__((packed));


struct wds_start_network {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
  struct apn_config apn;
  struct apn_type apn_type;
  struct ip_family_preference ip_family;
  struct profile_index profile;
  struct call_type call_type;
  struct autoconnect_config autoconnect;
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
#endif
