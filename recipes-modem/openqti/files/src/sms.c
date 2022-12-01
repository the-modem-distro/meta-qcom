// SPDX-License-Identifier: MIT

#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../inc/atfwd.h"
#include "../inc/call.h"
#include "../inc/cell_broadcast.h"
#include "../inc/command.h"
#include "../inc/config.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/proxy.h"
#include "../inc/qmi.h"
#include "../inc/sms.h"
#include "../inc/timesync.h"

/*
 * NOTE:
 *  This is pretty much just a prototype. There are lots of holes
 *  in the declared structures as there are lots of pieces of the
 *  different packets that I still haven't understood
 *
 * Take special care to the transaction IDs. The first one, which
 * generates the message notification, doesn't matter (initiated here)
 * After that, ModemManager has to actually request the pending message,
 * and it will come with a transaction ID. Not respecting that will make
 * ModemManager reject the following packets, so we need to track it and
 * only answer to it with what it expects.
 *
 *
 */

/*
 *  Array elem is #msg id
 *  pkt is whole packet
 *  send_state: 1 Notify | 2 Send | 3 DEL REQ | 4 Del SUCCESS
 *    On Del success wipe from array
 */
struct message {
  char pkt[MAX_MESSAGE_SIZE]; // JUST TEXT
  int len;                    // TEXT SIZE
  uint32_t message_id;
  uint8_t tp_dcs;
  uint8_t state; // message sending status
  uint8_t retries;
  struct timespec timestamp; // to know when to give up
};

struct message_queue {
  bool needs_intercept;
  int queue_pos;
  // max QUEUE_SIZE message to keep, we use the array as MSGID
  struct message msg[QUEUE_SIZE];
};

struct {
  bool notif_pending;
  uint8_t source;
  uint32_t current_message_id;
  uint16_t curr_transaction_id;
  struct message_queue queue;
} sms_runtime;

void reset_sms_runtime() {
  sms_runtime.notif_pending = false;
  sms_runtime.curr_transaction_id = 0;
  sms_runtime.source = -1;
  sms_runtime.queue.queue_pos = -1;
  sms_runtime.current_message_id = 0;
}

void set_notif_pending(bool pending) { sms_runtime.notif_pending = pending; }

void set_pending_notification_source(uint8_t source) {
  sms_runtime.source = source;
}

uint8_t get_notification_source() { return sms_runtime.source; }

bool is_message_pending() { return sms_runtime.notif_pending; }

int gsm7_to_ascii(const unsigned char *buffer, int buffer_length,
                  char *output_sms_text, int sms_text_length) {
  int output_text_length = 0;
  if (buffer_length > 0)
    output_sms_text[output_text_length++] = BITMASK_7BITS & buffer[0];

  int carry_on_bits = 1;
  int i = 1;
  for (; i < buffer_length; ++i) {

    output_sms_text[output_text_length++] =
        BITMASK_7BITS &
        ((buffer[i] << carry_on_bits) | (buffer[i - 1] >> (8 - carry_on_bits)));

    if (output_text_length == sms_text_length)
      break;

    carry_on_bits++;

    if (carry_on_bits == 8) {
      carry_on_bits = 1;
      output_sms_text[output_text_length++] = buffer[i] & BITMASK_7BITS;
      if (output_text_length == sms_text_length)
        break;
    }
  }
  if (output_text_length < sms_text_length) // Add last remainder.
    output_sms_text[output_text_length++] =
        buffer[i - 1] >> (8 - carry_on_bits);

  return output_text_length;
}

static inline void Write7Bits(uint8_t *bufferPtr, uint8_t val, uint32_t pos) {
  val &= 0x7F;
  uint8_t idx = pos / 8;

  if (!(pos & 7)) {
    bufferPtr[idx] = val;
  } else if ((pos & 7) == 1) {
    bufferPtr[idx] = bufferPtr[idx] | (val << 1);
  } else {
    bufferPtr[idx] = bufferPtr[idx] | (val << (pos & 7));
    bufferPtr[idx + 1] = (val >> (8 - (pos & 7)));
  }
}
/**
 * Convert an ascii array into a 7bits array
 * length is the number of bytes in the ascii buffer
 *
 * @return the size of the a7bit string (in 7bit chars!), or LE_OVERFLOW if
 * a7bitPtr is too small.
 */
uint8_t ascii_to_gsm7(const uint8_t *a8bitPtr, ///< [IN] 8bits array to convert
                      uint8_t *a7bitPtr        ///< [OUT] 7bits array result
) {
  int read;
  int write = 0;
  int size = 0;
  int pos = 0;
  int length = strlen((char *)a8bitPtr);

  for (read = pos; read < length + pos; ++read) {
    uint8_t byte = Ascii8to7[a8bitPtr[read]];

    /* Escape */
    if (byte >= 128) {
      Write7Bits(a7bitPtr, 0x1B, write * 7);
      write++;
      byte -= 128;
    }

    Write7Bits(a7bitPtr, byte, write * 7);
    write++;

    /* Number of 8 bit chars */
    size = (write * 7);
    size = (size % 8 != 0) ? (size / 8 + 1) : (size / 8);
  }

  return write;
}

uint8_t swap_byte(uint8_t source) {
  uint8_t parsed = 0;
  parsed = (parsed << 4) + (source % 10);
  parsed = (parsed << 4) + (int)(source / 10);
  return parsed;
}

int decode_phone_number(uint8_t *buffer, int len, char *out) {
  char output[127];
  int i = 0, j = 0;
  bool is_international = false;
  while (i < len) {
    if (i == 0 &&
        buffer[j] ==
            0x91) { // 0x91 == international number, no need to print it
      j++;
      is_international = true;
    }
    if (i % 2 == 0) {
      output[i] = (buffer[j] & 0x0f) + '0';
    } else {
      output[i] = ((buffer[j] & 0xf0) >> 4) + '0';
      j++;
    }
    i++;
  }
  output[len] = '\0';
  if (is_international) {
    snprintf(out, 128, "+%s", output);
  } else {
    snprintf(out, 128, "%s", output);
  }
  return len;
}
/*
 * This sends a notification message, ModemManager should answer it
 * with a request to get the actual message
 */
uint8_t generate_message_notification(int fd, uint32_t message_id) {
  struct wms_message_indication_packet *notif_pkt;
  notif_pkt = calloc(1, sizeof(struct wms_message_indication_packet));
  sms_runtime.curr_transaction_id = 0;
  notif_pkt->qmuxpkt.version = 0x01;
  notif_pkt->qmuxpkt.packet_length =
      sizeof(struct wms_message_indication_packet) - 1; // SIZE UNTESTED!
  notif_pkt->qmuxpkt.control = 0x80;
  notif_pkt->qmuxpkt.service = 0x05;
  notif_pkt->qmuxpkt.instance_id = 0x01;

  notif_pkt->qmipkt.ctlid = 0x04;
  notif_pkt->qmipkt.transaction_id = 2;
  notif_pkt->qmipkt.msgid = WMS_EVENT_REPORT;
  notif_pkt->qmipkt.length = sizeof(struct sms_storage_type) +
                             sizeof(struct sms_message_mode) +
                             sizeof(struct sms_over_ims);

  notif_pkt->storage.tlv_message_type = TLV_MESSAGE_TYPE;
  notif_pkt->storage.tlv_msg_type_size = htole16(5);
  notif_pkt->storage.storage_type = 0x01; // we simulate modem storage
  notif_pkt->storage.message_id = message_id;

  notif_pkt->mode.tlv_message_mode = TLV_MESSAGE_MODE;
  notif_pkt->mode.tlv_mode_size = htole16(1);
  notif_pkt->mode.message_mode = 0x01; // GSM

  notif_pkt->ims.tlv_sms_on_ims = TLV_SMS_OVER_IMS;
  notif_pkt->ims.tlv_sms_on_ims_size = htole16(1);
  notif_pkt->ims.is_sms_sent_over_ims = 0x00; // Nah, we don't

  if (write(fd, notif_pkt, sizeof(struct wms_message_indication_packet)) < 0) {
    logger(MSG_ERROR, "%s: Error sending new message notification\n", __func__);
  } else {
    logger(MSG_DEBUG, "%s: Sent new message notification\n", __func__);
  }
  dump_pkt_raw((uint8_t *)notif_pkt,
               sizeof(struct wms_message_indication_packet));
  free(notif_pkt);
  notif_pkt = NULL;
  return 0;
}

/* After sending a message to ModemManager, it asks for the message deletion
 * We need to build a packet with struct wms_message_delete_packet
 * and *sometimes* send it twice, once with QMI result 0x01 0x32
 * and another one with 0x00 0x00
 */
uint8_t process_message_deletion(int fd, uint32_t message_id,
                                 uint8_t indication) {
  struct wms_message_delete_packet *ctl_pkt;
  ctl_pkt = calloc(1, sizeof(struct wms_message_delete_packet));

  ctl_pkt->qmuxpkt.version = 0x01;
  ctl_pkt->qmuxpkt.packet_length = sizeof(struct wms_message_delete_packet) - 1;
  ctl_pkt->qmuxpkt.control = 0x80;
  ctl_pkt->qmuxpkt.service = 0x05;
  ctl_pkt->qmuxpkt.instance_id = 0x01;

  ctl_pkt->qmipkt.ctlid = 0x02;
  ctl_pkt->qmipkt.transaction_id = sms_runtime.curr_transaction_id;
  ctl_pkt->qmipkt.msgid = WMS_DELETE;
  ctl_pkt->qmipkt.length = 0x07; // SIZE

  ctl_pkt->indication.result_code_type = TLV_QMI_RESULT;
  ctl_pkt->indication.generic_result_size = 0x04; // uint32_t
  if (indication == 0) {
    ctl_pkt->indication.result = 0x01;
    ctl_pkt->indication.response = 0x32;
  } else if (indication == 1) {
    ctl_pkt->indication.result = 0x00;
    ctl_pkt->indication.response = 0x00;
  }

  if (write(fd, ctl_pkt, sizeof(struct wms_message_delete_packet)) < 0) {
    logger(MSG_ERROR, "%s: Error deleting message\n", __func__);
  }

  free(ctl_pkt);
  ctl_pkt = NULL;
  return 0;
}
/*
 * Build and send SMS
 *  Gets message ID, builds the QMI messages and sends it
 *  Returns numnber of bytes sent.
 *  Since oFono tries to read the message an arbitrary number
 *  of times, or delete it or whatever, we need to keep them
 *  a little longer on hold...
 */
int build_and_send_message(int fd, uint32_t message_id) {
  struct wms_build_message *this_sms;
  this_sms = calloc(1, sizeof(struct wms_build_message));
  int ret, fullpktsz;
  uint8_t tmpyear;

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  uint8_t msgoutput[160] = {0};
  ret = ascii_to_gsm7((uint8_t *)sms_runtime.queue.msg[message_id].pkt,
                      msgoutput);
  logger(MSG_DEBUG, "%s: Bytes to write %i\n", __func__, ret);
  /* QMUX */
  this_sms->qmuxpkt.version = 0x01;
  this_sms->qmuxpkt.packet_length = 0x00; // SIZE
  this_sms->qmuxpkt.control = 0x80;
  this_sms->qmuxpkt.service = 0x05;
  this_sms->qmuxpkt.instance_id = 0x01;
  /* QMI */
  this_sms->qmipkt.ctlid = 0x02;
  this_sms->qmipkt.transaction_id = sms_runtime.curr_transaction_id;
  this_sms->qmipkt.msgid = WMS_READ_MESSAGE;
  this_sms->qmipkt.length = 0x00; // SIZE
  /* INDICATION */
  this_sms->indication.result_code_type = TLV_QMI_RESULT;
  this_sms->indication.generic_result_size = 0x04;
  this_sms->indication.result = 0x00;
  this_sms->indication.response = 0x00;
  /* MESSAGE SETTINGS */
  this_sms->header.message_tlv = 0x01;
  this_sms->header.size =
      0x00; //  this_sms->unknown_data.wms_sms_size = 0x00; // SIZE
  this_sms->header.tlv_version = 0x01; // 3GPP

  this_sms->data.tlv = 0x06;
  // SMSC NUMBER SIZE RAW, we leave it hardcoded
  /* We shouldn't need to worry too much about the SMSC
   * since we're not actually sending this but...
   */
  /* SMSC */
  this_sms->data.smsc.phone_number_size =
      0x07; // hardcoded as we use a dummy one
  this_sms->data.smsc.is_international_number = 0x91; // yes
  this_sms->data.smsc.number[0] = 0x22;               // 0x51;
  this_sms->data.smsc.number[1] = 0x33;
  this_sms->data.smsc.number[2] = 0x44;
  this_sms->data.smsc.number[3] = 0x55;
  this_sms->data.smsc.number[4] = 0x66;
  this_sms->data.smsc.number[5] = 0x77;

  this_sms->data.unknown = 0x04; // This is still unknown

  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   * 4 bits for each number, backwards  */
  /* PHONE NUMBER */
  this_sms->data.phone.phone_number_size = 0x0c;       // hardcoded
  this_sms->data.phone.is_international_number = 0x91; // yes

  this_sms->data.phone.number[0] = 0x22; // 0x51;
  this_sms->data.phone.number[1] = 0x33;
  this_sms->data.phone.number[2] = 0x44;
  this_sms->data.phone.number[3] = 0x55;
  this_sms->data.phone.number[4] = 0x66;
  this_sms->data.phone.number[5] = 0x77;
  /* Unsure of these */

  this_sms->data.tp_pid = 0x00;
  this_sms->data.tp_dcs = 0x00;

  /*
   * tm_year should return number of years from 1900
   * If time hasn't synced yet it will say we're in
   * the 70s, so we don't know the correct date yet
   * In this case, we fall back to 2022, otherwise
   * the message would be end up being shown as
   * received in 2070.
   */
  if (tm.tm_year > 100) {
    tmpyear = tm.tm_year - 100;
  } else {
    tmpyear = 22;
  }
  /* DATE TIME */
  this_sms->data.date.year = swap_byte(tmpyear);
  this_sms->data.date.month = swap_byte(tm.tm_mon + 1);
  this_sms->data.date.day = swap_byte(tm.tm_mday);
  this_sms->data.date.hour = swap_byte(tm.tm_hour);
  this_sms->data.date.minute = swap_byte(tm.tm_min);
  this_sms->data.date.second = swap_byte(tm.tm_sec);
  this_sms->data.date.timezone = swap_byte(get_timezone() * 4);
  if (is_timezone_offset_negative()) {
    this_sms->data.date.timezone |= 1 << 3;
  }

  /* CONTENTS */
  memcpy(this_sms->data.contents.contents, msgoutput, ret);
  /* SIZES AND LENGTHS */

  // Total packet size to send
  fullpktsz = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
              sizeof(struct qmi_generic_result_ind) +
              sizeof(struct wms_raw_message_header) +
              sizeof(struct wms_user_data) - MAX_MESSAGE_SIZE +
              ret; // ret == msgsize
  // QMUX packet size
  this_sms->qmuxpkt.packet_length =
      fullpktsz - sizeof(uint8_t); // ret == msgsize, last uint qmux ctlid
  // QMI SZ: Full packet - QMUX header
  this_sms->qmipkt.length = sizeof(struct qmi_generic_result_ind) +
                            sizeof(struct wms_raw_message_header) +
                            sizeof(struct wms_user_data) - MAX_MESSAGE_SIZE +
                            ret;
  // Header size: QMI - indication size - uint16_t size element itself - header
  // tlv
  this_sms->header.size = this_sms->qmipkt.length -
                          sizeof(struct qmi_generic_result_ind) -
                          (3 * sizeof(uint8_t));
  // User size: QMI - indication - header - uint16_t size element - own tlv
  this_sms->data.user_data_size =
      this_sms->qmipkt.length - sizeof(struct qmi_generic_result_ind) -
      sizeof(struct wms_raw_message_header) - (3 * sizeof(uint8_t));

  /* Content size is the number of bytes _after_ conversion
   * from GSM7 to ASCII bytes (not the actual size of string)
   */

  this_sms->data.contents.content_sz =
      strlen((char *)sms_runtime.queue.msg[message_id].pkt);

  ret = write(fd, (uint8_t *)this_sms, fullpktsz);
  dump_pkt_raw((uint8_t *)this_sms, fullpktsz);

  free(this_sms);
  this_sms = NULL;
  return ret;
}

/*
 * Build and send SMS without touching the contents (for UCS2/8B encoded messages)
 *  Gets message ID, builds the QMI messages and sends it
 *  Returns numnber of bytes sent.
 *  Since oFono tries to read the message an arbitrary number
 *  of times, or delete it or whatever, we need to keep them
 *  a little longer on hold...
 */
int build_and_send_raw_message(int fd, uint32_t message_id) {
  struct wms_build_message *this_sms;
  this_sms = calloc(1, sizeof(struct wms_build_message));
  int ret, fullpktsz;
  uint8_t tmpyear;

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  /* QMUX */
  this_sms->qmuxpkt.version = 0x01;
  this_sms->qmuxpkt.packet_length = 0x00; // SIZE
  this_sms->qmuxpkt.control = 0x80;
  this_sms->qmuxpkt.service = 0x05;
  this_sms->qmuxpkt.instance_id = 0x01;
  /* QMI */
  this_sms->qmipkt.ctlid = 0x02;
  this_sms->qmipkt.transaction_id = sms_runtime.curr_transaction_id;
  this_sms->qmipkt.msgid = WMS_READ_MESSAGE;
  this_sms->qmipkt.length = 0x00; // SIZE
  /* INDICATION */
  this_sms->indication.result_code_type = TLV_QMI_RESULT;
  this_sms->indication.generic_result_size = 0x04;
  this_sms->indication.result = 0x00;
  this_sms->indication.response = 0x00;
  /* MESSAGE SETTINGS */
  this_sms->header.message_tlv = 0x01;
  this_sms->header.size =
      0x00; //  this_sms->unknown_data.wms_sms_size = 0x00; // SIZE
  this_sms->header.tlv_version = 0x01; // 3GPP

  this_sms->data.tlv = 0x06;
  // SMSC NUMBER SIZE RAW, we leave it hardcoded
  /* We shouldn't need to worry too much about the SMSC
   * since we're not actually sending this but...
   */
  /* SMSC */
  this_sms->data.smsc.phone_number_size =
      0x07; // hardcoded as we use a dummy one
  this_sms->data.smsc.is_international_number = 0x91; // yes
  this_sms->data.smsc.number[0] = 0x22;               // 0x51;
  this_sms->data.smsc.number[1] = 0x33;
  this_sms->data.smsc.number[2] = 0x44;
  this_sms->data.smsc.number[3] = 0x55;
  this_sms->data.smsc.number[4] = 0x66;
  this_sms->data.smsc.number[5] = 0x77;

  // ENCODING TEST
  this_sms->data.unknown = 0x04; // This is still unknown

  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   * 4 bits for each number, backwards  */
  /* PHONE NUMBER */
  this_sms->data.phone.phone_number_size = 0x0c;       // hardcoded
  this_sms->data.phone.is_international_number = 0x91; // yes

  this_sms->data.phone.number[0] = 0x22; // 0x51;
  this_sms->data.phone.number[1] = 0x33;
  this_sms->data.phone.number[2] = 0x44;
  this_sms->data.phone.number[3] = 0x55;
  this_sms->data.phone.number[4] = 0x66;
  this_sms->data.phone.number[5] = 0x77;
  /* Unsure of these */

  this_sms->data.tp_pid = 0x00;
  this_sms->data.tp_dcs = 0x00;
  if (sms_runtime.queue.msg[message_id].tp_dcs != 0x00) {
    logger(MSG_INFO, "%s: UCS/8Bit %.2x\n", __func__, sms_runtime.queue.msg[message_id].tp_dcs);
    this_sms->data.tp_dcs = sms_runtime.queue.msg[message_id].tp_dcs;
    if (sms_runtime.queue.msg[message_id].len < MAX_MESSAGE_SIZE && sms_runtime.queue.msg[message_id].len % 2 != 0) {
      logger(MSG_WARN, "%s: Uneven message size!\n", __func__);
      sms_runtime.queue.msg[message_id].len++;
    } 
  }
    /* CONTENTS */
  memcpy(this_sms->data.contents.contents,
         sms_runtime.queue.msg[message_id].pkt,
         sms_runtime.queue.msg[message_id].len);

  /*
   * tm_year should return number of years from 1900
   * If time hasn't synced yet it will say we're in
   * the 70s, so we don't know the correct date yet
   * In this case, we fall back to 2022, otherwise
   * the message would be end up being shown as
   * received in 2070.
   */
  if (tm.tm_year > 100) {
    tmpyear = tm.tm_year - 100;
  } else {
    tmpyear = 22;
  }
  /* DATE TIME */
  this_sms->data.date.year = swap_byte(tmpyear);
  this_sms->data.date.month = swap_byte(tm.tm_mon + 1);
  this_sms->data.date.day = swap_byte(tm.tm_mday);
  this_sms->data.date.hour = swap_byte(tm.tm_hour);
  this_sms->data.date.minute = swap_byte(tm.tm_min);
  this_sms->data.date.second = swap_byte(tm.tm_sec);
  this_sms->data.date.timezone = swap_byte(get_timezone() * 4);
  if (is_timezone_offset_negative()) {
    this_sms->data.date.timezone |= 1 << 3;
  }

  /* SIZES AND LENGTHS */

  // Total packet size to send
  fullpktsz = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
              sizeof(struct qmi_generic_result_ind) +
              sizeof(struct wms_raw_message_header) +
              sizeof(struct wms_user_data) - MAX_MESSAGE_SIZE +
              sms_runtime.queue.msg[message_id].len; // ret == msgsize
  // QMUX packet size
  this_sms->qmuxpkt.packet_length =
      fullpktsz - sizeof(uint8_t); // ret == msgsize, last uint qmux ctlid
  // QMI SZ: Full packet - QMUX header
  this_sms->qmipkt.length = sizeof(struct qmi_generic_result_ind) +
                            sizeof(struct wms_raw_message_header) +
                            sizeof(struct wms_user_data) - MAX_MESSAGE_SIZE +
                            sms_runtime.queue.msg[message_id].len;
  // Header size: QMI - indication size - uint16_t size element itself - header
  // tlv
  this_sms->header.size = this_sms->qmipkt.length -
                          sizeof(struct qmi_generic_result_ind) -
                          (3 * sizeof(uint8_t));
  // User size: QMI - indication - header - uint16_t size element - own tlv
  this_sms->data.user_data_size =
      this_sms->qmipkt.length - sizeof(struct qmi_generic_result_ind) -
      sizeof(struct wms_raw_message_header) - (3 * sizeof(uint8_t));

  /* In this case we leave the size alone, this ain't gsm-7 */
  this_sms->data.contents.content_sz =
      sms_runtime.queue.msg[message_id].len;

  ret = write(fd, (uint8_t *)this_sms, fullpktsz);
  dump_pkt_raw((uint8_t *)this_sms, fullpktsz);

  free(this_sms);
  this_sms = NULL;
  return ret;
}

/*
 * 1. Send new message notification
 * 2. Wait for answer from the Pinephone for a second (retry if no answer)
 * 3. Send message to pinephone
 * 4. Wait 2 ack events
 * 5. Respond 2 acks
 */
/*  QMI device should be the USB socket here, we are talking
 *  in private with out host, ADSP doesn't need to know
 *  anything about this
 *  This func does the entire transaction
 */
int handle_message_state(int fd, uint32_t message_id) {
  if (message_id > QUEUE_SIZE) {
    logger(MSG_ERROR, "%s: Attempting to read invalid message ID: %i\n",
           __func__, message_id);
    return 0;
  }

  logger(MSG_DEBUG, "%s: Attempting to handle message ID: %i\n", __func__,
         message_id);

  switch (sms_runtime.queue.msg[message_id].state) {
  case 0: // Generate -> RECEIVE TID
    logger(MSG_DEBUG, "%s: Notify Message ID: %i\n", __func__, message_id);
    generate_message_notification(fd, message_id);
    clock_gettime(CLOCK_MONOTONIC,
                  &sms_runtime.queue.msg[message_id].timestamp);
    sms_runtime.queue.msg[message_id].state = 1;
    sms_runtime.current_message_id =
        sms_runtime.queue.msg[message_id].message_id;
    break;
  case 1: // GET TID AND MOVE to 2
    logger(MSG_DEBUG, "%s: Waiting for ACK %i : state %i\n", __func__,
           message_id, sms_runtime.queue.msg[message_id].state);
    break;
  case 2: // SEND MESSAGE AND WAIT FOR TID
    logger(MSG_DEBUG, "%s: Send message. Message ID: %i\n", __func__,
           message_id);
    if (sms_runtime.queue.msg[message_id].tp_dcs == 0x00) {
      if (build_and_send_message(fd, message_id) > 0) {
        sms_runtime.queue.msg[message_id].state = 3;
      } else {
        logger(MSG_WARN, "%s: Failed to send message ID: %i\n", __func__,
               message_id);
      }
    } else {
      if (build_and_send_raw_message(fd, message_id) > 0) {
        sms_runtime.queue.msg[message_id].state = 3;
      } else {
        logger(MSG_WARN, "%s: Failed to send message ID: %i\n", __func__,
               message_id);
      }
    }
    clock_gettime(CLOCK_MONOTONIC,
                  &sms_runtime.queue.msg[message_id].timestamp);
    break;
  case 3: // GET TID AND DELETE MESSAGE
    logger(MSG_DEBUG, "%s: Waiting for ACK %i: state %i\n", __func__,
           message_id, sms_runtime.queue.msg[message_id].state);
    break;
  case 4:
    logger(MSG_DEBUG, "%s: ACK Deletion. Message ID: %i\n", __func__,
           message_id);
    if (sms_runtime.queue.msg[message_id].len > 0) {
      process_message_deletion(fd, 0, 0);
    } else {
      process_message_deletion(fd, 0, 1);
    }
    clock_gettime(CLOCK_MONOTONIC,
                  &sms_runtime.queue.msg[message_id].timestamp);
    sms_runtime.queue.msg[message_id].state = 9;
    memset(sms_runtime.queue.msg[message_id].pkt, 0, MAX_MESSAGE_SIZE);
    sms_runtime.queue.msg[message_id].len = 0;
    sms_runtime.current_message_id++;
    break;
  default:
    logger(MSG_WARN, "%s: Unknown task for message ID: %i (%i) \n", __func__,
           message_id, sms_runtime.queue.msg[message_id].state);
    break;
  }
  return 0;
}
void wipe_queue() {
  logger(MSG_DEBUG, "%s: Wipe status. \n", __func__);
  for (int i = 0; i <= sms_runtime.queue.queue_pos; i++) {
    sms_runtime.queue.msg[i].state = 0;
    sms_runtime.queue.msg[i].retries = 0;
  }
  set_notif_pending(false);
  set_pending_notification_source(MSG_NONE);
  sms_runtime.queue.queue_pos = -1;
  sms_runtime.current_message_id = 0;
}

/*
 *  We'll end up here from the proxy when a WMS packet is received
 *  and MSG_INTERNAL is still active. We'll assume current_message_id
 *  is where we need to operate
 */
void notify_wms_event(uint8_t *bytes, size_t len, int fd) {
  int offset;
  struct encapsulated_qmi_packet *pkt;
  pkt = (struct encapsulated_qmi_packet *)bytes;
  sms_runtime.curr_transaction_id = pkt->qmi.transaction_id;
  logger(MSG_INFO, "%s: Messages in queue: %i\n", __func__,
         sms_runtime.queue.queue_pos + 1);
  if (sms_runtime.queue.queue_pos < 0) {
    logger(MSG_DEBUG, "%s: Nothing to do \n", __func__);
    return;
  }

  switch (pkt->qmi.msgid) {
  case WMS_EVENT_REPORT:
    logger(
        MSG_WARN,
        "%s: WMS_EVENT_REPORT for message %i. ID %.4x (SHOULDNT BE CALLED)\n",
        __func__, sms_runtime.current_message_id, pkt->qmi.msgid);
    break;
  case WMS_RAW_SEND:
    logger(MSG_DEBUG, "%s: WMS_RAW_SEND for message %i. ID %.4x\n", __func__,
           sms_runtime.current_message_id, pkt->qmi.msgid);
    break;
  case WMS_RAW_WRITE:
    logger(MSG_DEBUG, "%s: WMS_RAW_WRITE for message %i. ID %.4x\n", __func__,
           sms_runtime.current_message_id, pkt->qmi.msgid);
    break;
  case WMS_READ_MESSAGE:
    /*
     * ModemManager got the indication and is requesting the message.
     * So let's clear it out
     */
    logger(MSG_INFO, "%s: Requesting message contents for ID %i\n", __func__,
           sms_runtime.current_message_id);
    if (len >= sizeof(struct wms_request_message)) {
      dump_pkt_raw(bytes, len);
      struct wms_request_message *request;
      request = (struct wms_request_message *)bytes;
      offset = get_tlv_offset_by_id(bytes, len, 0x01);
      if (offset > 0) {
        struct sms_storage_type *storage;
        storage = (struct sms_storage_type *)(bytes + offset);
        logger(MSG_DEBUG,
               "%s: Calculated from offset: 0x%.2x, from request: 0x%.2x\n",
               __func__, storage->message_id, request->storage.message_id);
      }
      logger(MSG_DEBUG, "%s: TLV ID for this query should be 0x%.2x\n",
             __func__, request->message_tag.id);
      if (request->message_tag.id == 0x01) {
        logger(MSG_DEBUG,
               "oFono gives us the message id tlv in a different order...\n ");
        struct wms_request_message_ofono *req2 =
            (struct wms_request_message_ofono *)bytes;
        sms_runtime.current_message_id = req2->storage.message_id;
        req2 = NULL;
      } else {
        sms_runtime.current_message_id = request->storage.message_id;
      }
      request = NULL;
      sms_runtime.queue.msg[sms_runtime.current_message_id].state = 2;
      handle_message_state(fd, sms_runtime.current_message_id);
      clock_gettime(
          CLOCK_MONOTONIC,
          &sms_runtime.queue.msg[sms_runtime.current_message_id].timestamp);
      //    request = NULL;
    } else {
      logger(MSG_DEBUG,
             "%s: WMS_READ_MESSAGE cannot proceed: Packet too small\n",
             __func__);

      dump_pkt_raw(bytes, len);
    }

    break;
  case WMS_DELETE:
    logger(MSG_DEBUG, "%s: WMS_DELETE for message %i. ID %.4x\n", __func__,
           sms_runtime.current_message_id, pkt->qmi.msgid);
    if (sms_runtime.queue.msg[sms_runtime.current_message_id].state != 3) {
      logger(MSG_INFO, "%s: Requested to delete previous message \n", __func__);
      if (sms_runtime.current_message_id > 0) {
        sms_runtime.current_message_id--;
      }
    }
    sms_runtime.queue.msg[sms_runtime.current_message_id].state = 4;
    handle_message_state(fd, sms_runtime.current_message_id);
    clock_gettime(
        CLOCK_MONOTONIC,
        &sms_runtime.queue.msg[sms_runtime.current_message_id].timestamp);
    break;
  default:
    logger(MSG_DEBUG, "%s: Unknown event received: %.4x\n", __func__,
           pkt->qmi.msgid);

    break;
  }
  pkt = NULL;
}

/*
 * Process message queue
 *  We'll end up here from the proxy, when a MSG_INTERNAL is
 *  pending, but not necessarily as a response to a host WMS query
 *
 */
int process_message_queue(int fd) {
  int i;
  struct timespec cur_time;
  double elapsed_time;

  clock_gettime(CLOCK_MONOTONIC, &cur_time);

  if (sms_runtime.queue.queue_pos < 0) {
    logger(MSG_INFO, "%s: Nothing yet \n", __func__);
    return 0;
  }

  if (sms_runtime.current_message_id > sms_runtime.queue.queue_pos + 1) {
    logger(MSG_INFO, "%s: We finished the queue \n", __func__);
  }

  if (sms_runtime.queue.queue_pos >= 0) {
    for (i = 0; i <= sms_runtime.queue.queue_pos; i++) {

      elapsed_time =
          (((cur_time.tv_sec - sms_runtime.queue.msg[i].timestamp.tv_sec) *
            1e9) +
           (cur_time.tv_nsec - sms_runtime.queue.msg[i].timestamp.tv_nsec)) /
          1e9;
      if (elapsed_time < 0) {
        clock_gettime(CLOCK_MONOTONIC, &sms_runtime.queue.msg[i].timestamp);
      }
      switch (sms_runtime.queue.msg[i].state) {
      case 0: // We're beginning, we need to send the notification
        sms_runtime.current_message_id = sms_runtime.queue.msg[i].message_id;
        handle_message_state(fd, sms_runtime.current_message_id);
        return 0;
      case 2: // For whatever reason we're here with a message send pending
        handle_message_state(fd, sms_runtime.current_message_id);
        return 0;
      case 4:
        handle_message_state(fd, sms_runtime.current_message_id);
        return 0;
      case 1: // We're here but we're waiting for an ACK
      case 3:
        if (elapsed_time > 5 && sms_runtime.queue.msg[i].retries < 3) {
          logger(MSG_WARN, "-->%s: Retrying message id %i \n", __func__, i);
          sms_runtime.queue.msg[i].retries++;
          sms_runtime.queue.msg[i].state--;
        } else if (elapsed_time > 5 && sms_runtime.queue.msg[i].retries >= 3) {
          logger(MSG_ERROR, "-->%s: Message %i timed out, killing it \n",
                 __func__, i);
          memset(sms_runtime.queue.msg[i].pkt, 0, MAX_MESSAGE_SIZE);
          sms_runtime.queue.msg[i].state = 9;
          sms_runtime.queue.msg[i].retries = 0;
          sms_runtime.queue.msg[i].len = 0;
          sms_runtime.current_message_id++;
        } else {
          logger(MSG_WARN, "-->%s: Waiting on message for %i \n", __func__, i);
        }
        return 0;
      }
    }
  }

  logger(MSG_INFO, "%s: Nothing left in the queue \n", __func__);
  wipe_queue();
  return 0;
}

/*
 * Update message queue and add new message text
 * to the array
 */
void add_sms_to_queue(uint8_t *message, size_t len) {
  if (sms_runtime.queue.queue_pos > QUEUE_SIZE - 2) {
    logger(MSG_ERROR, "%s: Queue is full!\n", __func__);
    return;
  }
  if (len > 0) {
    set_notif_pending(true);
    set_pending_notification_source(MSG_INTERNAL);
    logger(MSG_INFO, "%s: Adding message to queue (%i)\n", __func__,
           sms_runtime.queue.queue_pos + 1);
    sms_runtime.queue.queue_pos++;
    memcpy(sms_runtime.queue.msg[sms_runtime.queue.queue_pos].pkt, message,
           len);
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].message_id =
        sms_runtime.queue.queue_pos;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].tp_dcs = 0x00;
  } else {
    logger(MSG_ERROR, "%s: Size of message is 0\n", __func__);
  }
}
void add_raw_sms_to_queue(uint8_t *message, size_t len, uint8_t tp_dcs) {
  if (sms_runtime.queue.queue_pos > QUEUE_SIZE - 2) {
    logger(MSG_ERROR, "%s: Queue is full!\n", __func__);
    return;
  }
  if (len > 0) {
    set_notif_pending(true);
    set_pending_notification_source(MSG_INTERNAL);
    logger(MSG_INFO, "%s: Adding message to queue (%i)\n", __func__,
           sms_runtime.queue.queue_pos + 1);
    sms_runtime.queue.queue_pos++;
    memcpy(sms_runtime.queue.msg[sms_runtime.queue.queue_pos].pkt, message,
           len);
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].message_id = sms_runtime.queue.queue_pos;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].len = len;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].tp_dcs = tp_dcs;
    logger(MSG_INFO, "RAW MESSAGE: %s -> %s | %i\n", message, sms_runtime.queue.msg[sms_runtime.queue.queue_pos].pkt, sms_runtime.queue.msg[sms_runtime.queue.queue_pos].len);
  } else {
    logger(MSG_ERROR, "%s: Size of message is 0\n", __func__);
  }
}

/* Generate a notification indication */
uint8_t do_inject_notification(int fd) {
  set_notif_pending(false);
  set_pending_notification_source(MSG_NONE);
  generate_message_notification(fd, 0);
  return 0;
}

/*
 * AT+SIMUSMS will call this to add some text messages
 * to the queue
 */
uint8_t inject_message(uint8_t message_id) {
  add_message_to_queue((uint8_t *)"Hello world!", strlen("Hello world!"));
  return 0;
}

uint8_t send_outgoing_msg_ack(uint16_t transaction_id, int usbfd,
                              uint16_t message_id) {
  int ret;
  struct sms_received_ack *receive_ack;
  receive_ack = calloc(1, sizeof(struct sms_received_ack));
  receive_ack->qmuxpkt.version = 0x01;
  receive_ack->qmuxpkt.packet_length = 0x0018; // SIZE
  receive_ack->qmuxpkt.control = 0x80;
  receive_ack->qmuxpkt.service = 0x05;
  receive_ack->qmuxpkt.instance_id = 0x01;

  receive_ack->qmipkt.ctlid = 0x02;
  receive_ack->qmipkt.transaction_id = transaction_id;
  receive_ack->qmipkt.msgid = WMS_RAW_SEND;
  receive_ack->qmipkt.length = 0x000c; // SIZE
  receive_ack->indication.result_code_type = TLV_QMI_RESULT;
  receive_ack->indication.generic_result_size = htole16(4);
  receive_ack->indication.result = 0x00;
  receive_ack->indication.response = 0x00;

  receive_ack->message_tlv_id = 0x01;
  receive_ack->message_id_len = 0x0002;
  receive_ack->message_id =
      0x0021; // this one gets ignored both by ModemManager and oFono
  logger(MSG_DEBUG, "%s: Sending Host->Modem SMS ACK\n", __func__);
  dump_pkt_raw((uint8_t *)receive_ack, sizeof(struct sms_received_ack));
  ret = write(usbfd, receive_ack, sizeof(struct sms_received_ack));
  free(receive_ack);
  return ret;
}

/* Intercept and ACK a message */
uint8_t intercept_and_parse(void *bytes, size_t len, int adspfd, int usbfd) {
  uint8_t *output;
  uint8_t ret;
  struct outgoing_sms_packet *pkt;
  struct outgoing_no_validity_period_sms_packet *nodate_pkt;

  output = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    nodate_pkt = (struct outgoing_no_validity_period_sms_packet *)bytes;
    /* This will need to be rebuilt for oFono, probably
     *  0x31 -> Most of ModemManager stuff
     *  0x11 -> From jeremy, still keeps 0x21
     *  0x01 -> Skips the 0x21 and jumps to content
     */
    if (pkt->pdu_type >= 0x11) {
      ret = gsm7_to_ascii(pkt->contents.contents,
                          strlen((char *)pkt->contents.contents),
                          (char *)output, pkt->contents.content_sz);
      if (ret < 0) {
        logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
               __LINE__);
      }
    } else if (pkt->pdu_type == 0x01) {
      ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                          strlen((char *)nodate_pkt->contents.contents),
                          (char *)output, nodate_pkt->contents.content_sz);
      if (ret < 0) {
        logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
               __LINE__);
      }
    } else {
      set_log_level(0);

      logger(MSG_ERROR,
             "%s: Don't know how to handle this. Please contact biktorgj and "
             "get him the following dump:\n",
             __func__);
      dump_pkt_raw(bytes, len);
      logger(MSG_ERROR,
             "%s: Don't know how to handle this. Please contact biktorgj and "
             "get him the following dump:\n",
             __func__);
      set_log_level(1);
    }

    send_outgoing_msg_ack(pkt->qmipkt.transaction_id, usbfd, 0x0000);
    parse_command(output);
  }
  pkt = NULL;
  nodate_pkt = NULL;
  free(output);
  return 0;
}

int pdu_decode(uint8_t *buffer, int buffer_length,
               char *output_sender_phone_number, uint8_t *output_sms_text) {
  if (buffer_length <= 0)
    return -1;

  int sender_phone_number_size = 128;
  int sms_text_size = 160;
  const int sms_deliver_start = 1 + buffer[0];
  if (sms_deliver_start + 1 > buffer_length)
    return -1;
  if ((buffer[sms_deliver_start] & 0x04) != 0x04) {
    logger(MSG_ERROR, "%s: buf not 0x04\n", __func__);
    return -1;
  }

  const int sender_number_length = buffer[sms_deliver_start + 1];
  if (sender_number_length + 1 > sender_phone_number_size)
    return -1; // Buffer too small to hold decoded phone number.

  decode_phone_number(buffer + sms_deliver_start + 3, sender_number_length,
                      output_sender_phone_number);

  const int sms_pid_start =
      sms_deliver_start + 3 + (buffer[sms_deliver_start + 1] + 1) / 2;
  logger(MSG_ERROR, "%s: Looking for contents\n", __func__);

  const int sms_start = sms_pid_start + 2 + 7;
  if (sms_start + 1 > buffer_length) {
    logger(MSG_ERROR, "%s: Invalid input buffer\n", __func__);
    return -1; // Invalid input buffer.
  }

  const int output_sms_text_length = buffer[sms_start];
  if (sms_text_size < output_sms_text_length) {
    logger(MSG_ERROR, "%s: Cant hold buffer\n", __func__);
    return -1; // Cannot hold decoded buffer.
  }
  const int decoded_sms_text_size =
      gsm7_to_ascii(buffer + sms_start + 1, buffer_length - (sms_start + 1),
                    (char *)output_sms_text, output_sms_text_length);

  if (decoded_sms_text_size != output_sms_text_length) {
    logger(MSG_ERROR, "%s: Invalid decoded length\n", __func__);

    return -1; // Decoder length is not as expected.
  }

  // Add a C string end.
  if (output_sms_text_length < sms_text_size)
    output_sms_text[output_sms_text_length] = 0;
  else
    output_sms_text[sms_text_size - 1] = 0;

  return output_sms_text_length;
}

/* Sniff on an sms */
uint8_t log_message_contents(uint8_t source, void *bytes, size_t len) {
  uint8_t *output;
  uint8_t ret;
  char phone_numb[128];
  output = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  if (source == FROM_HOST) {
    struct outgoing_sms_packet *pkt;
    struct outgoing_no_validity_period_sms_packet *nodate_pkt;
    if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
      pkt = (struct outgoing_sms_packet *)bytes;
      nodate_pkt = (struct outgoing_no_validity_period_sms_packet *)bytes;
      ret = decode_phone_number(pkt->target.phone_number, pkt->target.sz,
                                phone_numb);
      /* This will need to be rebuilt for oFono, probably
       *  0x31 -> Most of ModemManager stuff
       *  0x11 -> From jeremy, still keeps 0x21
       *  0x01 -> Skips the 0x21 and jumps to content
       */
      if (pkt->pdu_type >= 0x11) {
        ret = gsm7_to_ascii(pkt->contents.contents,
                            strlen((char *)pkt->contents.contents),
                            (char *)output, pkt->contents.content_sz);
        if (ret < 0) {
          logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
                 __LINE__);
        }
      } else if (pkt->pdu_type == 0x01) {
        ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                            strlen((char *)nodate_pkt->contents.contents),
                            (char *)output, nodate_pkt->contents.content_sz);
        if (ret < 0) {
          logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
                 __LINE__);
        }
      } else {
        logger(MSG_ERROR, "%s: Unimplemented PDU type (0x%.2x)\n", __func__,
               pkt->pdu_type);
      }
      logger(MSG_INFO, "[SMS] From User to %s | Contents: %s\n", phone_numb,
             output);
    }
    pkt = NULL;
    nodate_pkt = NULL;
  } else if (source == FROM_DSP) {
    int offset = get_tlv_offset_by_id(bytes, len, 0x01);
    if (offset > 0) {
      struct empty_tlv *pdu = (struct empty_tlv *)(bytes + offset);
      /*
      |------------------------------------------------------------------------|
      |4 byte |  1-12 bytes   | 1  | 2-12 |  1  |  1  | 0||1||7 | 1   | 0-140  |
      |------------------------------------------------------------------------|
      | ???   |SCA | PDU Type | MR |  DA  | PID | DCS |    VP   | UDL |   UD   |
      |------------------------------------------------------------------------|
      */
      pdu_decode(pdu->data + 4, le16toh(pdu->len) - 4, phone_numb, output);
      pdu = NULL;
      logger(MSG_INFO, "[SMS] From %s to User | Contents: %s\n", phone_numb,
             output);

    } else {
      logger(MSG_ERROR, "%s: Failed to find PDU offset from the data TLV\n",
             __func__);
    }
  }
  free(output);
  return 0;
}
int check_wms_message(uint8_t source, void *bytes, size_t len, int adspfd,
                      int usbfd) {
  uint8_t our_phone[] = {0x91, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  int needs_rerouting = 0;
  struct outgoing_sms_packet *pkt;
  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    // is it for us?
    if (memcmp(pkt->target.phone_number, our_phone,
               sizeof(pkt->target.phone_number)) == 0) {
      logger(MSG_DEBUG, "%s: We got a message \n", __func__);
      intercept_and_parse(bytes, len, adspfd, usbfd);
      needs_rerouting = 1;
    }
    if (is_sms_logging_enabled() && (pkt->qmipkt.msgid == WMS_READ_MESSAGE ||
                                     pkt->qmipkt.msgid == WMS_RAW_SEND)) {
      log_message_contents(source, bytes, len);
    }
  }
  return needs_rerouting;
}

int check_wms_indication_message(void *bytes, size_t len, int adspfd,
                                 int usbfd) {
  int needs_pass_through = 0;
  struct wms_message_indication_packet *pkt;
  if (len >= sizeof(struct wms_message_indication_packet)) {
    pkt = (struct wms_message_indication_packet *)bytes;
    // is it for us?
    if (pkt->qmipkt.msgid == WMS_EVENT_REPORT &&
        get_transceiver_suspend_state()) {
      logger(MSG_INFO, "%s: Attempting to wake up the host", __func__);
      pulse_ring_in(); // try to wake the host
      sleep(3);        // sleep for 5s
      // Enqueue an incoming notification
      set_pending_notification_source(MSG_EXTERNAL);
      set_notif_pending(true);
      needs_pass_through = 1;
      set_sms_notification_pending_state(true);
    }
  }
  return needs_pass_through;
}

/* Intercept and ACK a message */
uint8_t intercept_cb_message(void *bytes, size_t len) {
  uint8_t *output;
  uint8_t ret;
  uint8_t strsz = 0;
  struct cell_broadcast_message_prototype *pkt;
  uint8_t *reply = calloc(MAX_CB_MESSAGE_SIZE, sizeof(unsigned char));

  output = calloc(MAX_CB_MESSAGE_SIZE, sizeof(uint8_t));

  if (len >= sizeof(struct cell_broadcast_message_prototype) -
                 (MAX_CB_MESSAGE_SIZE + 2)) {
    logger(MSG_INFO, "%s: Message size is big enough\n", __func__);

    pkt = (struct cell_broadcast_message_prototype *)bytes;
    if (pkt->header.id == TLV_TRANSFER_MT_MESSAGE) {
      logger(MSG_INFO, "%s: TLV ID matches, trying to decode\n", __func__);
      logger(MSG_WARN,
             "CB Message dump:\n--\n"
             " ID: 0x%.2x\n "
             " LEN: 0x%.4x\n "
             " - Serial %i\n"
             " - Message ID %i\n"
             " - Encoding: 0x%.2x\n"
             " - Page %.2x\n",
             pkt->header.id, 
             pkt->header.len, 
             pkt->message.pdu.serial_number,
             pkt->message.pdu.message_id, 
             pkt->message.pdu.encoding,
             pkt->message.pdu.page_param);

      set_log_level(0);
      logger(MSG_DEBUG, "%s: CB MESSAGE DUMP\n", __func__);
      dump_pkt_raw(bytes, len);
      logger(MSG_DEBUG, "%s: CB MESSAGE DUMP END\n", __func__);
      set_log_level(1);
      if ((pkt->message.pdu.page_param >> 4) == 0x01)
        add_message_to_queue((uint8_t *)"WARNING: Incoming Cell Broadcast Message from the network", strlen("WARNING: Incoming Cell Broadcast Message from the network"));

      // If binary for encoding is 01xx xxxx, then we have encoding data
      // Otherwise we assume it's something else and encoding is GSM7
      if (!(pkt->message.pdu.encoding & (1 << 7))  && (pkt->message.pdu.encoding & (1 << 6))) { // 01xx
            logger(MSG_WARN, "%s:Message encoding is not GSM-7\n", __func__);
            // cell_broadcast_header - 10 == remaining size of the pkt from content start... always
            int sz = pkt->header.len;
            if (sz > MAX_MESSAGE_SIZE)
             memcpy(output, pkt->message.pdu.contents, MAX_MESSAGE_SIZE);
            else 
             memcpy(output, pkt->message.pdu.contents, sz);

            int sms_dcs = pkt->message.pdu.encoding;
            /* We need to make sure certain bits of the data coding scheme are set to 0 to
             * avoid confusing ModemManager...
               https://www.etsi.org/deliver/etsi_ts/123000_123099/123038/10.00.00_60/ts_123038v100000p.pdf
              We leave alone bit #5 (Compression), and #3 and #2, which define the encoding type (GSM7 || 8bit || UCS2)
              We clear the rest
             */
            sms_dcs &= ~(1UL << 7); // We already know encoding is set to 01xx (CB, section 5, page 12), SMS needs
            sms_dcs &= ~(1UL << 6); // this to be to 00xx (SMS, Section 4, page 8) to match data coding scheme
//            sms_dcs &= ~(1UL << 5); // We leave this bit as is, *compression*
            sms_dcs &= ~(1UL << 4); // We clear the message class bit, we don't need this
//            sms_dcs &= ~(1UL << 3); // We keep these, as they set the encoding type
//            sms_dcs &= ~(1UL << 2); // GSM7 / UCS2 / TE Specific
            sms_dcs &= ~(1UL << 1); // We clear these, as they are related to bit 4
            sms_dcs &= ~(1UL << 0); // we just cleared before.
            logger(MSG_WARN, "%s: Setting DCS from %.2x to %.2x\n",__func__, pkt->message.pdu.encoding, sms_dcs);
            add_raw_sms_to_queue(output, sz, sms_dcs);

          } else {
            ret = gsm7_to_ascii(pkt->message.pdu.contents,
                                  (pkt->message.len - 6), // == sizeof(cell_broadcast_message_pdu -6 byte from the serial, message id, encoding and page)
                                  (char *)output, pkt->message.len); // 2 ui16, 2 ui8
              if (ret < 0) {
                logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
                      __LINE__);
              } else {
                strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n", output);
                add_message_to_queue(reply, strsz);
              }
          }
     
    }
  }
  pkt = NULL;
  free(output);
  return 0;
}

int check_cb_message(void *bytes, size_t len, int adspfd, int usbfd) {
  struct cell_broadcast_message_prototype *pkt;
  if (len >= sizeof(struct cell_broadcast_message_prototype) -
                 (MAX_CB_MESSAGE_SIZE + 2)) {
    pkt = (struct cell_broadcast_message_prototype *)bytes;
    if (pkt->qmipkt.msgid == WMS_EVENT_REPORT &&
        pkt->header.id == TLV_TRANSFER_MT_MESSAGE &&
        pkt->message.id != 0x00) { /* Note below */
      logger(MSG_INFO, "%s: We got a CB message? \n", __func__);
      intercept_cb_message(bytes, len);
    }
  }
  return 0; // We let it go anyway
}

/*
So far we only managed to catch 1 Cell broadcast message, and
two MMS Event reports.

The Cell Broadcast message had a PDU ID of 0x07, while both MMS
event reports had a 0x00 in that TLV ID. So, until I know more,
and without trying to decode the entire PDU, we care about everything
which is *not* a 0x00 */