/* SPDX-License-Identifier: MIT */

#ifndef _CALL_H_
#define _CALL_H_
#include "../inc/qmi.h"
#include "sms.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_PHONE_NUMBER_LENGTH 20
#define MAX_ACTIVE_CALLS 10
/*
 *  <-- qmi.h [struct qmux_packet]
 *  <-- qmi.h [struct qmi_packet]
 *  <-- qmi.h [struct qmi_generic_result_ind]
 */

/* QMI message IDs for Voice calls*/
enum {
  VO_SVC_CALL_REQUEST = 0x0020,
  VO_SVC_CALL_END_REQ = 0x0021,
  VO_SVC_CALL_ANSWER_REQ = 0x0022,
  VO_SVC_CALL_INFO = 0x0024,
  VO_SVC_CALL_STATUS = 0x002e,
  VO_SVC_GET_ALL_CALL_INFO = 0x002f, // Requested during calls sometimes
  VO_SVC_CODEC_INFO = 0x0053,
  VO_SVC_CALL_HANDOVER = 0x0054,
  VO_SVC_CALL_CONF_INFO = 0x0055,
  VO_SVC_CALL_CONF_JOIN = 0x0056,
  VO_SVC_CONF_PARTICIPANT_UPDATE = 0x0057,
  VO_SVC_CONF_PARTICIPANT_INFO = 0x005b,
  VO_SVC_CALL_CONTROL = 0x005a,
  VO_SVC_CALL_STATUS_CHANGE = 0x0032,
};

struct call_request_tlv {
  uint8_t tlvid;   // 0x01
  uint16_t length; // 0x09 for spanish number without prefix
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
} __attribute__((packed));

struct call_status_meta { // dont know what this is
  uint8_t tlvid;          // 0x01
  uint16_t length;        // 0x08 0x00
  uint8_t unk1;
  uint8_t unk2;
  uint8_t call_state;
  uint8_t unk4;
  uint8_t call_direction;
  uint8_t call_type;
  uint8_t unk7;
  uint8_t unk8;
  // no fucking clue: 0x01 0x01 0x02 0x02 0x02 0x04 0x00 0x00
} __attribute__((packed));

struct unknown_indication2 {
  uint8_t tlvid;   // 0x27
  uint16_t length; // 0x03 0x00
  uint8_t data[3]; // 0x01 0x01 0x02 no fucking clue
} __attribute__((packed));

struct participant_info {
  uint8_t tlvid;   // 0x10
  uint16_t length; // 0x0d 0x00
  uint8_t call_no; // UNKNOWN, seems call number
} __attribute__((packed));

struct unknown_indication3 {
  uint8_t tlvid;           // 0x2b
  uint16_t length;         // 0x10 for spanish numb no int
  uint8_t unknown_data[5]; // 0x01 0x01 0x00 0x00 0x00 0x01
} __attribute__((packed));

struct remote_number {
  uint8_t tlvid;             // 0x01
  uint16_t phone_num_length; // 0x09
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
} __attribute__((packed));

struct call_request_remote_number {
  uint8_t tlvid;             // 0x2b || 0x01 <req
  uint16_t phone_num_length; // 0x09
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
} __attribute__((packed));

struct call_status_indicat {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Unknown data */
  struct call_status_meta meta;
  /* Participant info */
  struct participant_info participant;
  /* Unknown 2 */
  struct unknown_indication2 indication2;
  /* Unknown 3 */
  struct unknown_indication3 indication3;
  /* Remote party number */
  struct remote_number number;

} __attribute__((packed));

struct call_status_indication {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* No idea */
  struct call_status_meta meta;
  /* Call info */
  struct remote_number number;
} __attribute__((packed));

struct call_request_indication {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Remote party number */
  struct call_request_remote_number number;

} __attribute__((packed));

struct call_data {
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
  uint8_t direction;
  uint8_t state;
  uint8_t call_type;
};

struct call_fake_request_remote_number {
  uint8_t tlvid; // 0x10
  uint16_t len;
  uint8_t data[3];
  uint8_t phone_num_length; // 0x0d
  uint8_t phone_number[13];
} __attribute__((packed));

struct voice_call_info {
  uint8_t id;   // 0x27
  uint16_t len; // 0x03;
  uint8_t data[3];
} __attribute__((packed));

struct secure_call_tlv {
  uint8_t id;      // 0x32
  uint16_t len;    // 0x03
  uint8_t data[3]; // 0x01 0x01 0x00
} __attribute__((packed));

/* OPTIONAL, TRY REMOVING IT */
struct ip_call_tlv {
  uint8_t id;   // 0x22
  uint16_t len; // 0x1a
  uint8_t data[26];
} __attribute__((packed));

struct uss_data_tlv {
  uint8_t id;   // 0x21
  uint16_t len; // 0x1a
  uint8_t data[26];
} __attribute__((packed));

/* Used by get_all_call_info */
struct caller_id_info {
  uint8_t id; // 0x11
  uint16_t len;
  uint8_t data[3];           // 0x01 0x01 0x00
  uint8_t phone_number_size; // 0x0c?
  uint8_t phone_number[13];
} __attribute__((packed));
struct remote_party_extension_tlv {
  uint8_t id;
  uint16_t len;
  uint8_t data[6];
  uint8_t phone_number_size; // 0x0c?
  uint8_t phone_number[13];
} __attribute__((packed));

struct simulated_call_packet {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Metadata */
  struct call_status_meta meta; // 0x01, 8
  /* Remote party number */
  struct call_fake_request_remote_number number; // 0x10

  struct voice_call_info info; // 0x27
  struct remote_party_extension_tlv remote_party_extension;
} __attribute__((packed));

struct call_id_tlv {
  uint8_t id;   // 0x10
  uint16_t len; // 1
  uint8_t data;
} __attribute__((packed));

struct media_id_tlv {
  uint8_t id;   // 0x15
  uint16_t len; // 1
  uint8_t data;
} __attribute__((packed));

struct call_request_response_packet {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Generic result indication */
  struct qmi_generic_result_ind indication;
  /* Call UID 0x10 --> 0x01->0xFF? */
  struct call_id_tlv call_id;
  /* Media ID 0x15 */
  struct media_id_tlv media_id; // 0x03 always?
} __attribute__((packed));

struct all_call_info_packet {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Generic result indication */
  struct qmi_generic_result_ind indication; // 0x02,4 0 0 0 0
  /* Metadata */
  struct call_status_meta meta; // 0x10 0x08
  /* Caller ID information */
  struct caller_id_info caller_id; // 0x11
  /* Remote number */
  struct remote_party_extension_tlv remote_party_extension; // 0x25
} __attribute__((packed));

struct end_call_request {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Generic result indication */
  struct qmi_generic_result_ind indication; // 0x00,4 1 1 0 1
} __attribute__((packed));

struct end_call_response {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Generic result indication */
  struct qmi_generic_result_ind indication; // 0x02,4 0 0 0 0
  /* Call ID */
  struct call_id_tlv call_id;
} __attribute__((packed));

struct call_accept_ack {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Generic result indication */
  struct qmi_generic_result_ind indication; // 0x02,4 0 0 0 0
  /* Call ID */
  struct call_id_tlv call_id;
} __attribute__((packed));
/* Functions */
void reset_call_state();
void set_pending_call_flag(bool en);
uint8_t get_call_pending();
void set_call_simulation_mode(bool en);
uint8_t get_call_simulation_mode();
void start_simulated_call(int usbfd);
void *can_you_hear_me();
void send_call_request_response(int usbfd, uint16_t transaction_id);
uint8_t send_voice_call_status_event(int usbfd, uint16_t transaction_id,
                                     uint8_t call_direction,
                                     uint8_t call_state);
void send_dummy_call_established(int usbfd, uint16_t transaction_id);
uint8_t call_service_handler(uint8_t source, void *bytes, size_t len,
                             uint16_t msgid, int adspfd, int usbfd);

void add_voice_message_to_queue(uint8_t *message, size_t len);
#endif
