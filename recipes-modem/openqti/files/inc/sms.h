/* SPDX-License-Identifier: MIT */

#ifndef _SMS_H
#define _SMS_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/qmi.h"


/*
 *  MSG IDs
 * 0x0002 -> New messages
 * 0x0022 -> Message data
 * 0x0024 -> ACK of message to delete it in ME?
 *
 *  <-- ipc.h [struct qmux_packet]
 *  <-- ipc.h [struct qmi_packet]
 */
// Could be anything: 0x02 0x04 0x00 0x00 0x00 0x00 0x00 0x01
struct unknown_interim_data { 
    uint8_t unk1; // 0x02
    uint8_t unk2; // 0x04
    uint8_t unk3; // 0x00
    uint8_t unk4; // 0x00
    uint8_t unk5; // 0x00
    uint8_t unk6; // 0x00
    uint8_t unk7; // 0x00
    uint8_t unk8; // 0x01
    uint16_t wms_sms_size; // Size of the remaining part of the packet so far
    uint8_t unk9;  // 0x01
    uint8_t unk10; // 0x06
    uint16_t rest_of_pkt_len;
} __attribute__((packed));

struct smsc_data {
    uint16_t sz;
    uint8_t smsc[7];
} __attribute__((packed));

struct sms_caller_data {
    uint8_t unknown;
    uint16_t sz; // Size of phone number after converting to 8bit
    uint8_t *phone_number[11];
} __attribute__((packed));

struct sms_metadata {
    uint16_t unknown;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t seconds;
} __attribute__((packed));

struct sms_content {
    uint8_t unknown;
    uint8_t message_size_8bit;
    char *contents;
} __attribute__((packed));

struct sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct unknown_interim_data unknown_data;
    struct smsc_data incoming_smsc;  // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_caller_data caller; // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_metadata meta;
    struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

/* Functions */
void reset_sms_runtime();
bool is_message_pending();
void set_notif_pending(bool en);
uint8_t generate_message_notification(int fd, uint8_t pending_message_num);
uint8_t ack_message_notification(int fd, uint8_t pending_message_num);
uint8_t inject_message(int fd, uint8_t message_id);
#endif