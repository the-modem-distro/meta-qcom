// SPDX-License-Identifier: MIT
#ifndef _CELL_BROADCAST_H_
#define _CELL_BROADCAST_H_

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>

#define MAX_CB_MESSAGE_SIZE 1839

struct cell_broadcast_header {
    uint8_t id; // 0x11
    uint16_t len;
    uint8_t tp_pid; // Just a guess here, 0x00
    uint8_t tp_dcs; // 0xFF
    uint8_t unknown_params[3]; //  0xff 0xff 0xff -> Validity period? lang?
} __attribute__((packed));

struct cell_broadcast_message_pdu {

    uint16_t serial_number; // 6760
    uint16_t message_id; // 0x11 0x12
    uint8_t encoding; // 0x0f
    uint8_t page_param; // 66
    uint8_t contents[MAX_CB_MESSAGE_SIZE];
} __attribute__((packed));

struct cell_broadcast_message_pdu_container{
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