/* SPDX-License-Identifier: MIT */

#ifndef _SMS_H
#define _SMS_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/qmi.h"

#define MAX_MESSAGE_SIZE 160
#define QUEUE_SIZE 100
#define MAX_PHONE_NUMBER_SIZE 20

/* OpenQTI's way of knowing if it
  needs to trigger an internal or 
  external indication message */
enum {
    MSG_NONE = -1,
    MSG_EXTERNAL = 0,
    MSG_INTERNAL = 1,
};



/* QMI message IDs for SMS/MMS */
enum {
    WMS_RESET = 0x000,
    WMS_EVENT_REPORT = 0x0001,
    WMS_GET_SUPPORTED_MESSAGES = 0x001e,
    WMS_GET_SUPPORTED_FIELDS = 0x001f,
    WMS_RAW_SEND = 0x0020,
    WMS_RAW_WRITE = 0x0021,
    WMS_READ_MESSAGE = 0x0022,
    WMS_DELETE = 0x0024,
    WMS_GET_MSG_PROTOCOL = 0x0030,
};

/* For GSM7 message decoding */
enum {
	BITMASK_7BITS = 0x7F,
	BITMASK_8BITS = 0xFF,
	BITMASK_HIGH_4BITS = 0xF0,
	TYPE_OF_ADDRESS_INTERNATIONAL_PHONE = 0x91,
	TYPE_OF_ADDRESS_NATIONAL_SUBSCRIBER = 0xC8,
};

enum {
   TLV_QMI_RESULT = 0x02,
   TLV_MESSAGE_TYPE = 0x10,
   TLV_MESSAGE_MODE = 0x12,
   TLV_SMS_OVER_IMS = 0x16,
};

/*
 *  <-- qmi.h [struct qmux_packet]
 *  <-- qmi.h [struct qmi_packet]
 *  <-- qmi.h [struct qmi_generic_result_ind]
 */

struct sms_storage_type { // 8byte
    /* Message storage */
    uint8_t tlv_message_type; // 0x10
    uint16_t tlv_msg_type_size; //  5 bytes
    uint8_t storage_type; // 00 -> UIM, 01 -> NV
    uint32_t message_id; // Index! +CMTI: "ME", [MESSAGE_ID]
} __attribute__((packed));

struct sms_message_mode {
    /* Message mode */
    uint8_t tlv_message_mode; // 0x12
    uint16_t tlv_mode_size; // 0x01
    uint8_t message_mode; // 0x01 GSM
} __attribute__((packed));

struct sms_over_ims {
    /* SMS on IMS? */
    uint8_t tlv_sms_on_ims; // 0x16
    uint16_t tlv_sms_on_ims_size; // 0x01
    uint8_t is_sms_sent_over_ims; // 0x00 || 0x01 [no | yes]
} __attribute__((packed));

struct wms_message_indication_packet { // 0x0001
    /* QMUX header */
    struct qmux_packet qmuxpkt;
    /* QMI header */
    struct qmi_packet qmipkt;
    /* Storage - where is the message? */
    struct sms_storage_type storage;
    /* Mode - type of message? */
    struct sms_message_mode mode;
    /* SMS over IMS? Yes or no */
    struct sms_over_ims ims;
} __attribute__((packed));

struct wms_message_delete_packet { // 0x0024
    /* QMUX header */
    struct qmux_packet qmuxpkt;
    /* QMI header */
    struct qmi_packet qmipkt;
    /* Did we delete it from our (false) storage? */
    struct qmi_generic_result_ind indication;
} __attribute__((packed));

struct wms_message_settings {
    uint8_t message_tlv; // 0x01
    uint16_t size; // REMAINING SIZE OF PKT (!!)
 //??   uint8_t message_tag; // 0x00 read, 1 unread, 2 sent, 3, unsent, 4??
    uint8_t message_storage; // 0x00 || 0x01 (sim/mem?)
    uint8_t format; // always 0x06
} __attribute__((packed));

struct wms_raw_message_header {
    uint8_t message_tlv; // 0x01 RAW MSG
    uint16_t size; // REMAINING SIZE OF PKT (!!)
    uint8_t tlv_version; // 0x01
} __attribute__((packed));

struct generic_tlv_onebyte { // 4byte
    uint8_t id; // 0x01 RAW MSG
    uint16_t size; // REMAINING SIZE OF PKT (!!)
    uint8_t data; // 0x01
} __attribute__((packed));

struct wms_message_target_data {
    uint16_t message_size; // what remains in the packet

} __attribute__((packed));

struct wms_phone_number {
    uint8_t phone_number_size; // SMSC counts in gsm7, target in ascii (??!!?)
    uint8_t is_international_number; // 0x91 => +
    uint8_t number[MAX_PHONE_NUMBER_SIZE];
} __attribute__((packed));

struct wms_hardcoded_phone_number {
    uint8_t phone_number_size; // SMSC counts in gsm7, target in ascii (??!!?)
    uint8_t is_international_number; // 0x91 => +
    uint8_t number[6];
} __attribute__((packed));

struct wms_datetime {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timezone; // 0x40 ??
} __attribute__((packed));

struct wms_message_contents {
    uint8_t content_sz; // Size *AFTER* conversion
    uint8_t contents[MAX_MESSAGE_SIZE];
} __attribute__((packed));

struct wms_user_data {
    uint8_t tlv; // 0x06
    uint16_t user_data_size;
    struct wms_hardcoded_phone_number smsc; // source smsc
    uint8_t unknown; // 0x04 (!)
    struct wms_hardcoded_phone_number phone; // source numb
    uint8_t tp_pid; // Not sure at all, 0x00
    uint8_t tp_dcs; // Not sure at all, 0x00
    struct wms_datetime date;
    /* Actual data for the message */
    struct wms_message_contents contents;
} __attribute__((packed)); 

struct wms_build_message { 
    /* QMUX header */
    struct qmux_packet qmuxpkt;
    /* QMI header */
    struct qmi_packet qmipkt;
    /* Did it succeed? */
    struct qmi_generic_result_ind indication;
    /* This message tag and format */
    struct wms_raw_message_header header;
    /* Size of smsc + phone + date + tp* + contents */
    struct wms_user_data data;
} __attribute__((packed));

struct wms_request_message {
    /* QMUX header */
    struct qmux_packet qmuxpkt;
    /* QMI header */
    struct qmi_packet qmipkt;
    /* Message tag */
    struct generic_tlv_onebyte message_tag;
    /* Request data */
    struct sms_storage_type storage;
} __attribute__((packed));

struct wms_request_message_ofono {
    /* QMUX header */
    struct qmux_packet qmuxpkt;
    /* QMI header */
    struct qmi_packet qmipkt;
    /* Request data */
    struct sms_storage_type storage;
    /* Message tag */
    struct generic_tlv_onebyte message_tag;
} __attribute__((packed));


/* Messages outgoing from the
 * host to the modem. This needs
 * rebuilding too */

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

struct sms_content {
    uint8_t content_sz; // Size *AFTER* conversion
    uint8_t contents[MAX_MESSAGE_SIZE];
} __attribute__((packed));

struct outgoing_sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct sms_outgoing_header header; // RAW MESSAGE INDICATION
    struct sms_outgoing_header header_tlv2; // TYPE OF MESSAGE TO BE SENT, 0x06 -> 3GPP

    uint8_t sca_length; // 0x00 Indicates if it has a SMSC set. It shouldn't, we ignore it.
    uint8_t pdu_type; // 0xX1 == SUBMIT,  >=0x11 includes validity period
    struct sms_caller_data target; // 7bit gsm encoded htole, 0xf on last item if padding needed
    uint8_t tp_pid; // 0x00
    uint8_t tp_dcs; // 0x00
    uint8_t validity_period; // 0x21 || 0xa7
    struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

struct sms_received_ack {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct qmi_generic_result_ind indication;
  
    uint8_t user_data_tlv;
    uint16_t user_data_length;
    uint16_t user_data_value;
}__attribute__((packed));

struct outgoing_no_validity_period_sms_packet {
    struct qmux_packet qmuxpkt;
    struct qmi_packet qmipkt;
    struct sms_outgoing_header header;
    struct sms_outgoing_header header_tlv2;

    uint8_t sca_length; // 0x00 Indicates if it has a SMSC set. It shouldn't, we ignore it.
    uint8_t pdu_type; // 0xX1 == SUBMIT, >=0x11 includes validity period
    struct sms_caller_data target; // 7bit gsm encoded htole, 0xf on last item if padding needed
    uint16_t unk4; // 0x00 0x00
    struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

/* Functions */
void reset_sms_runtime();
void set_notif_pending(bool en);
void set_pending_notification_source(uint8_t source);
uint8_t get_notification_source();
bool is_message_pending();
uint8_t generate_message_notification(int fd, uint32_t message_id);
uint8_t ack_message_notification(int fd, uint8_t pending_message_num);
uint8_t inject_message(uint8_t message_id);
uint8_t do_inject_notification(int fd);

uint8_t intercept_and_parse(void *bytes, size_t len, uint8_t hostfd, uint8_t adspfd);


int process_message_queue(int fd);
void add_message_to_queue(uint8_t *message, size_t len);
void notify_wms_event(uint8_t *bytes,size_t len, int fd);
int check_wms_message(void *bytes, size_t len, uint8_t adspfd, uint8_t usbfd);
int check_wms_indication_message(void *bytes, size_t len, uint8_t adspfd,
                                 uint8_t usbfd);

#endif