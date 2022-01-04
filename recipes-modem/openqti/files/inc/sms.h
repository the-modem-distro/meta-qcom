/* SPDX-License-Identifier: MIT */

#ifndef _SMS_H
#define _SMS_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/qmi.h"

#define MSG_NOTIFICATION 0x0001
#define MSG_INDICATION 0x0022
#define MSG_CONTROL_ACK 0x0024
#define MAX_MESSAGE_SIZE 80

static const struct {
  unsigned int id;
  const char *text;
} sample_text[] = {
    {0, "Hello World!"},
    {1, "This is your modem talking!"},
    {2, "Hope you're having a great day!"},
    {3, "Now I can write you :)"},
    {4, "Hopefully, soon you'll be able to write me back!"},
    {5, "See you soon!"},
};

/*
 *  MSG IDs
 * 0x0002 -> New messages
 * 0x0022 -> Message data
 * 0x0024 -> ACK of message to delete it in ME?
 *
 *  <-- ipc.h [struct qmux_packet]
 *  <-- ipc.h [struct qmi_packet]
 * Too many unknowns to guess what they do
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
} __attribute__((packed));

struct sms_pkt_header {
    uint8_t unk9;  // 0x01
    uint8_t unk10; // 0x06
    uint16_t sms_content_sz;
}__attribute__((packed));

struct smsc_data {
    uint8_t sz;
    uint8_t smsc_number[7];
} __attribute__((packed));

struct sms_caller_data {
    uint8_t unknown;
    uint8_t sz; // Size of phone number *AFTER* converting to 8bit
    uint8_t phone_number[7];
} __attribute__((packed));

struct sms_metadata {
    uint16_t unknown;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t unknown2;
} __attribute__((packed));

struct sms_content {
    uint8_t content_sz; // Size *AFTER* conversion
    uint8_t contents[MAX_MESSAGE_SIZE];
} __attribute__((packed));

struct sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct unknown_interim_data unknown_data;
    struct sms_pkt_header header;
    struct smsc_data incoming_smsc;  // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_caller_data caller; // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_metadata meta;
    struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

struct sms_notif_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    uint8_t size;
    uint16_t service;
    uint16_t instance;
    uint8_t unkn4;
    uint8_t unkn5;
    uint8_t unkn6;
    uint8_t unkn7;
    uint8_t unkn8;
    uint8_t unkn9;
    uint8_t unkn10;
    uint8_t unkn11;
    uint8_t unkn12;
    uint8_t unkn13;
    uint8_t unkn14;
}__attribute__((packed));

struct sms_control_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    uint8_t unk1;
    uint8_t unk2;
    uint8_t unk3;
    uint8_t unk4;
    uint8_t unk5;
    uint8_t unk6;
    uint8_t unk7;
}__attribute__((packed));


/* Functions */
void reset_sms_runtime();
bool is_message_pending();
void set_notif_pending(bool en);
uint8_t generate_message_notification(int fd, uint8_t pending_message_num);
uint8_t ack_message_notification(int fd, uint8_t pending_message_num);
uint8_t inject_message(int fd, uint8_t message_id);
#endif