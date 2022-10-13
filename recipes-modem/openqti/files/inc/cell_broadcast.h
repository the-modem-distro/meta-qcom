// SPDX-License-Identifier: MIT
#ifndef _CELL_BROADCAST_H_
#define _CELL_BROADCAST_H_

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>

#define MAX_CB_MESSAGE_SIZE 1839
#define CB_ENABLE_AT_CMD "AT+CSCB=0,\"0-6000\",\"0-15\"\r"
#define CB_DISABLE_AT_CMD "AT+CSCB=1,\"0-6000\",\"0-15\"\r"

struct cell_broadcast_header {
  uint8_t id; // 0x11 // 3GPP Config
  uint16_t len; // Size
  uint16_t service_category;
  uint16_t language;
  uint8_t is_category_selected;
} __attribute__((packed));

struct cell_broadcast_message_pdu {

  uint16_t serial_number; // 6760
  uint16_t message_id;    // 0x11 0x12
  uint8_t encoding;       // 0x0f
  uint8_t page_param;     // 66
  uint8_t contents[MAX_CB_MESSAGE_SIZE];
} __attribute__((packed));

struct cell_broadcast_message_pdu_container {
  uint8_t id; // 0x07
  uint16_t len;
  struct cell_broadcast_message_pdu pdu;
} __attribute__((packed));

struct cell_broadcast_message_prototype {
  struct qmux_packet qmuxpkt;
  struct qmi_packet qmipkt;
  struct cell_broadcast_header header;
  struct cell_broadcast_message_pdu_container message;
} __attribute__((packed));

#endif
