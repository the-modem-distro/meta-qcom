// SPDX-License-Identifier: MIT
#include "../inc/qmi.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"
#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 *
 * Utilities for QMI messages
 *
 *
 *
 *
 *
 */

/* Get service */
uint8_t get_qmux_service_id(void *bytes, size_t len) {
  struct encapsulated_qmi_packet *pkt = (struct encapsulated_qmi_packet *)bytes;
  return pkt->qmux.service;
}

/* Get Message ID for a QMI message */
uint16_t get_message_id(void *bytes, size_t len) {
  struct encapsulated_qmi_packet *pkt = (struct encapsulated_qmi_packet *)bytes;
  return pkt->qmi.msgid;
}

/* Get Transaction ID for a QMI message */
uint16_t get_transaction_id(void *bytes, size_t len) {
  struct encapsulated_qmi_packet *pkt = (struct encapsulated_qmi_packet *)bytes;
  return pkt->qmi.transaction_id;
}

/* Looks for the request TLV, and if found, it returns the offset
 * so it can be casted later
 * Not all the modem handling daemons send TLVs in the same order,
 * This allows us to avoid having to hardcode every possible combination
 */
uint16_t get_tlv_offset_by_id(uint8_t *bytes, size_t len, uint8_t tlvid) {
  uint16_t cur_byte;
  uint8_t *arr = (uint8_t *)bytes;
  struct empty_tlv *this_tlv;
  if (len < sizeof(struct encapsulated_qmi_packet) + 4) {
    logger(MSG_ERROR, "%s: Packet is too small \n", __func__);
    return -EINVAL;
  }

  cur_byte = sizeof(struct encapsulated_qmi_packet);
  while ((cur_byte) < len) {
    this_tlv = (struct empty_tlv *)(arr + cur_byte);
    if (this_tlv->id == tlvid) {
      logger(MSG_DEBUG,
             "Found TLV with ID 0x%.2x at offset %i with size 0x%.4x\n",
             this_tlv->id, cur_byte, le16toh(this_tlv->len));
      arr = NULL;
      this_tlv = NULL;
      return cur_byte;
    }
    cur_byte += le16toh(this_tlv->len) + sizeof(uint8_t) + sizeof(uint16_t);
    if (cur_byte <= 0 || cur_byte > len) {
      logger(MSG_ERROR, "Current byte is less than or exceeds size\n");
      arr = NULL;
      this_tlv = NULL;
      return -EINVAL;
    }
  }
  arr = NULL;
  this_tlv = NULL;
  return -EINVAL;
}
 const char *get_qmi_error_string(uint16_t result_code) {
    for (int j = 0; j < (sizeof(qmi_error_codes) / sizeof(qmi_error_codes[0])); j++) {
      if (qmi_error_codes[j].code == result_code) {
        return qmi_error_codes[j].error_name;
      }
    }
  return "Unknown error";
}
uint16_t did_qmi_op_fail(uint8_t *bytes, size_t len) {
  uint16_t result = QMI_RESULT_FAILURE;
  uint16_t cur_byte;
  uint8_t *arr = (uint8_t *)bytes;
  struct qmi_generic_result_ind *this_tlv;

  if (len < sizeof(struct encapsulated_qmi_packet) + 4) {
    logger(MSG_ERROR, "%s: Packet is too small \n", __func__);
    return result;
  }

  cur_byte = sizeof(struct encapsulated_qmi_packet);
  while ((cur_byte) < len) {
    this_tlv = (struct qmi_generic_result_ind *)(arr + cur_byte);
    if (this_tlv->result_code_type == 0x02 && this_tlv->generic_result_size == 0x04) {
      result = this_tlv->result;
      if (this_tlv->result == QMI_RESULT_FAILURE) {
      logger(MSG_ERROR, "** QMI OP Failed: Code 0x%.4x (%s)\n", this_tlv->response, get_qmi_error_string(this_tlv->response));
      } else {
      logger(MSG_INFO, "** QMI OP Succeeded: Code 0x%.4x (%s)\n", this_tlv->response, get_qmi_error_string(this_tlv->response));
      }
      arr = NULL;

      this_tlv = NULL;
      return result;
    }
    cur_byte += le16toh(this_tlv->generic_result_size) + sizeof(uint8_t) + sizeof(uint16_t);
    if (cur_byte <= 0 || cur_byte > len) {
      logger(MSG_ERROR, "Current byte is less than or exceeds size\n");
      arr = NULL;
      this_tlv = NULL;
      return result;
    }
  }
  logger(MSG_WARN, "%s: Couldn't find an indication TLV\n", __func__);
  result = QMI_RESULT_UNKNOWN;

  arr = NULL;
  this_tlv = NULL;
  return result;
}