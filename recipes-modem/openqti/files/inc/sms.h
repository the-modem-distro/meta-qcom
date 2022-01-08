/* SPDX-License-Identifier: MIT */

#ifndef _SMS_H
#define _SMS_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/qmi.h"

#define MSG_NOTIFICATION 0x0001
#define MSG_ACK 0x0020
#define MSG_INDICATION 0x0022
#define MSG_CONTROL_ACK 0x0024
#define MAX_MESSAGE_SIZE 80

enum {
    MSG_NONE = -1,
    MSG_EXTERNAL = 0,
    MSG_INTERNAL = 1,
};
enum {
	BITMASK_7BITS = 0x7F,
	BITMASK_8BITS = 0xFF,
	BITMASK_HIGH_4BITS = 0xF0,
	BITMASK_LOW_4BITS = 0x0F,

	TYPE_OF_ADDRESS_INTERNATIONAL_PHONE = 0x91,
	TYPE_OF_ADDRESS_NATIONAL_SUBSCRIBER = 0xC8,

	SMS_DELIVER_ONE_MESSAGE = 0x04,
	SMS_SUBMIT              = 0x11,

	SMS_MAX_7BIT_TEXT_LENGTH  = 160,
};
static const struct {
  unsigned int id;
  const char *text;
} sample_text[] = {
    {0, "Hello Biktor"},
    {1, "How is your day going?"},
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

struct sms_incoming_header {
    uint8_t unk9;  // 0x01
    uint8_t unk10; // 0x06
    uint16_t sms_content_sz;
}__attribute__((packed));

struct sms_outgoing_header {
    uint8_t unknown; // 0x01
    uint16_t size1;
} __attribute__((packed));

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

struct incoming_sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct unknown_interim_data unknown_data;
    struct sms_incoming_header header;
    struct smsc_data incoming_smsc;  // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_caller_data caller; // 7bit gsm encoded htole, 0xf on last item if padding needed
    struct sms_metadata meta;
    struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

struct outgoing_sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct sms_outgoing_header header;
    struct sms_outgoing_header header_tlv2;

    uint8_t unk2; // 0x00
    uint8_t unk3; // 0x31
    struct sms_caller_data target; // 7bit gsm encoded htole, 0xf on last item if padding needed
    uint16_t unk4; // 0x00 0x00
    uint8_t date_tlv; // 0x21
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


struct sms_received_ack {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    uint8_t unk1;
    uint8_t unk2;
    uint16_t unk3;
    uint16_t unk4;
    uint8_t unk5;
    uint8_t unk6;
    uint16_t unk7;
    uint16_t unk8;
}__attribute__((packed));

/* Functions */
void reset_sms_runtime();
void set_notif_pending(bool en);
void set_pending_notification_source(uint8_t source);
uint8_t get_notification_source();
bool is_message_pending();
uint8_t generate_message_notification(int fd, uint8_t pending_message_num);
uint8_t ack_message_notification(int fd, uint8_t pending_message_num);
uint8_t inject_message(int fd, uint8_t message_id);


uint8_t intercept_and_parse(void *bytes, size_t len, uint8_t hostfd, uint8_t adspfd);
#endif