/* SPDX-License-Identifier: MIT */

#ifndef _QMI_H
#define _QMI_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/* QMUX (so far used in sms)
 *
 */
struct qmux_packet {      // 6 byte
  uint8_t version;        // 0x01 ?? it's always 0x01, no idea what it is
  uint16_t packet_length; // 0x44 0x00 -> Full size of the packet being received
  uint8_t control;        // 0x80 | 0x00
  uint8_t service;        // WMS is 0x05
  uint8_t instance_id; // Instance is usually 1 except for the control service
} __attribute__((packed));

struct qmi_packet { // 7 byte
  uint8_t ctlid;    // 0x00 | 0x02 | 0x04, subsystem inside message service?
  uint16_t transaction_id; // QMI Transaction ID
  uint16_t msgid; // QMI Message ID
  uint16_t length; // QMI Packet size
} __attribute__((packed));

struct qmi_generic_result_ind {
  uint8_t result_code_type;     // 0x02
  uint16_t generic_result_size; // 0x04 0x00
  uint16_t result;
  uint16_t response;
} __attribute__((packed));

struct ctl_qmi_packet {
  uint8_t ctlid; // 0x00 | 0x02 | 0x04, subsystem inside message service?
  uint8_t transaction_id; // QMI Transaction ID
  uint16_t
      msgid; // 0x0022 when message arrives, 0x0002 when there are new messages
  uint16_t length; // QMI Packet size
} __attribute__((packed));

struct encapsulated_qmi_packet {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
} __attribute__((packed));

struct encapsulated_control_packet {
  struct qmux_packet qmux;
  struct ctl_qmi_packet qmi;
} __attribute__((packed));

struct tlv_header {
  uint8_t id;   // 0x00
  uint16_t len; // 0x02 0x00
} __attribute__((packed));

struct signal_quality_tlv {
  uint8_t id;           // 0x10 = CDMA, 0x11 HDR, 0x12 GSM, 0x13 WCDMA, 0x14 LTE
  uint16_t len;         // We only care about the RSSI, but there's more stuff in here
  uint8_t signal_level; // RSSI
} __attribute__((packed));

struct nas_signal_lev {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Operation result */
  struct qmi_generic_result_ind result;
  /* Signal level data */
  struct signal_quality_tlv signal;

} __attribute__((packed));

enum {
  PACKET_EMPTY = 0,
  PACKET_PASS_TRHU,
  PACKET_BYPASS,
  PACKET_FORCED_PT,
};

struct empty_tlv {
  uint8_t id;
  uint16_t len;
  uint8_t data[0];
} __attribute__((packed));

struct tlv_position {
  uint8_t id;
  uint32_t offset;
  uint16_t size;
};

uint8_t get_qmux_service_id(void *bytes, size_t len);
uint16_t get_message_id(void *bytes, size_t len);
uint16_t get_transaction_id(void *bytes, size_t len);
uint16_t get_tlv_offset_by_id(uint8_t *bytes, size_t len, uint8_t tlvid);

#endif