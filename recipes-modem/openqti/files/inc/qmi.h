/* SPDX-License-Identifier: MIT */

#ifndef _QMI_H
#define _QMI_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/* QMUX (so far used in sms)
 *
 */
struct qmux_packet {
    uint8_t version; // 0x01 ?? it's always 0x01, no idea what it is
    uint16_t packet_length; // 0x44 0x00 -> Full size of the packet being received
    uint8_t control; // 0x80 | 0x00
    uint8_t service; // WMS is 0x05
    uint8_t instance_id; // Instance is usually 1 except for the control service
} __attribute__((packed));

struct qmi_packet {
  uint8_t ctlid;          // 0x00 | 0x02 | 0x04, subsystem inside message service?
  uint16_t transaction_id; // QMI Transaction ID
  uint16_t msgid;          // 0x0022 when message arrives, 0x0002 when there are new messages
  uint16_t length;         // QMI Packet size
} __attribute__((packed));


#endif