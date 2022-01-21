/* SPDX-License-Identifier: MIT */

#ifndef _CALL_H_
#define _CALL_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/qmi.h"

#define MAX_PHONE_NUMBER_LENGTH 20

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
    VO_SVC_CALL_HANDOVER = 0x0054,
    VO_SVC_CALL_CONF_INFO = 0x0055,
    VO_SVC_CALL_CONF_JOIN = 0x0056,
    VO_SVC_CONF_PARTICIPANT_UPDATE = 0x0057,
    VO_SVC_CONF_PARTICIPANT_INFO = 0x005b,
    VO_SVC_CALL_CONTROL = 0x005a,
};

enum {
   TLV_QMI_RESULT = 0x02,
   TLV_MESSAGE_TYPE = 0x10,
   TLV_MESSAGE_MODE = 0x12,
   TLV_SMS_OVER_IMS = 0x16,
};
struct call_request_tlv {
  uint8_t tlvid; // 0x01
  uint16_t length; // 0x09 for spanish number without prefix 
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
} __attribute__((packed));

struct call_status_indication {
  

} __attribute__((packed));

struct call_status_meta { // dont know what this is
  uint8_t tlvid; // 0x01
  uint16_t length; // 0x08 0x00
  uint8_t data[8]; // no fucking clue
} __attribute__((packed));

struct unknown_indication2 {
  uint8_t tlvid; // 0x27
  uint16_t length; // 0x03 0x00
  uint8_t data[3]; // 0x01 0x01 0x02 no fucking clue
} __attribute__((packed));

struct participant_info {
  uint8_t tlvid; // 0x10
  uint16_t length; // 0x0d 0x00
  uint8_t call_no; // UNKNOWN, seems call number
} __attribute__((packed));

struct remote_number {
    uint8_t tlvid; // 0x2b
    uint16_t length; // 0x10 for spanish numb no int
    uint8_t unknown_data[6]; // 0x01 0x01 0x00 0x00 0x00 0x01
    uint8_t phone_num_length; // 0x09
    uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
} __attribute__((packed));

struct call_request_indication {
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
    /* Remote party number */
    struct remote_number number;

} __attribute__((packed));

#endif