// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../inc/command.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/sms.h"

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

struct {
  bool notif_pending;
  uint8_t source;
  uint16_t curr_transaction_id;
  uint8_t demo_text_no;
} sms_runtime;

struct messages {
  char message[160];
  uint8_t send_state;
};
struct {
  bool needs_intercept;
  struct messages msg[10]; // max 10 message to keep, we use the array as MSGID
} message_platform_status;

void reset_sms_runtime() {
  sms_runtime.notif_pending = false;
  sms_runtime.curr_transaction_id = 0;
  sms_runtime.demo_text_no = 0;
  sms_runtime.source = -1;
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

uint8_t ascii_to_gsm7(const uint8_t *in, uint8_t *out) {
  unsigned bit_count = 0;
  unsigned bit_queue = 0;
  uint8_t bytes_written = 0;
  while (*in) {
    bit_queue |= (*in & 0x7Fu) << bit_count;
    bit_count += 7;
    if (bit_count >= 8) {
      *out++ = (uint8_t)bit_queue;
      bytes_written++;
      bit_count -= 8;
      bit_queue >>= 8;
    }
    in++;
  }
  if (bit_count > 0) {
    *out++ = (uint8_t)bit_queue;
    bytes_written++;
  }

  return bytes_written;
}

uint8_t swap_byte(uint8_t source) {
  uint8_t parsed = 0;
  parsed = (parsed << 4) + (source % 10);
  parsed = (parsed << 4) + (int)(source/10);
  return parsed;
}

/*
 * This sends a notification message, ModemManager should answer it
 * with a request to get the actual message
 */
uint8_t generate_message_notification(int fd, uint32_t message_id) {
  int ret;
  struct wms_message_indication_packet *notif_pkt;
  notif_pkt = calloc(1, sizeof(struct wms_message_indication_packet));
  notif_pkt->qmuxpkt.version = 0x01;
  notif_pkt->qmuxpkt.packet_length = sizeof(struct wms_message_indication_packet) -1; // SIZE UNTESTED!
  notif_pkt->qmuxpkt.control = 0x80;
  notif_pkt->qmuxpkt.service = 0x05;
  notif_pkt->qmuxpkt.instance_id = 0x01;

  notif_pkt->qmipkt.ctlid = 0x04;
  notif_pkt->qmipkt.transaction_id = 2;
  notif_pkt->qmipkt.msgid = WMS_EVENT_REPORT;
  notif_pkt->qmipkt.length = sizeof(struct sms_storage_type) + sizeof(struct sms_message_mode) + sizeof(struct sms_over_ims);
  
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

  ret = write(fd, notif_pkt, sizeof(struct wms_message_indication_packet));
  logger(MSG_INFO, "%s: Sent new message notification\n", __func__);
  dump_pkt_raw((uint8_t *)notif_pkt, sizeof(struct wms_message_indication_packet));
  sms_runtime.curr_transaction_id++;
  free(notif_pkt);
  notif_pkt = NULL;
  return 0;
}
/* After sending a message to ModemManager, it asks for the message deletion
 * We need to build a packet with struct wms_message_delete_packet
 * and send it twice, once with QMI result 0x01 0x32
 * and another one with 0x00 0x00
 */
uint8_t process_message_deletion(int fd, uint32_t message_id, uint8_t indication) {
  struct wms_message_delete_packet *ctl_pkt;
  int ret;
  ctl_pkt = calloc(1, sizeof(struct wms_message_delete_packet));

  ctl_pkt->qmuxpkt.version = 0x01;
  ctl_pkt->qmuxpkt.packet_length = sizeof(struct wms_message_delete_packet)-1;
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
  
  ret = write(fd, ctl_pkt, sizeof(struct wms_message_delete_packet));

  free(ctl_pkt);
  ctl_pkt = NULL;
  return 0;
}

int build_and_send_sms(int fd, uint8_t *msg) {
  struct wms_build_message *this_sms;
  this_sms = calloc(1, sizeof(struct wms_build_message));
  int ret, fullpktsz;
  uint8_t tmpyear;

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  uint8_t msgoutput[160] = {0};
  ret = ascii_to_gsm7(msg, msgoutput);
  logger(MSG_INFO, "%s: Bytes to write %i\n", __func__, ret);
  /* QMUX */
  this_sms->qmuxpkt.version = 0x01;
  this_sms->qmuxpkt.packet_length = 0x00; // SIZE
  this_sms->qmuxpkt.control = 0x80;
  this_sms->qmuxpkt.service = 0x05;
  this_sms->qmuxpkt.instance_id = 0x01;
  /* QMI */
  this_sms->qmipkt.ctlid = 0x0002;
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
  this_sms->header.size = 0x00; //  this_sms->unknown_data.wms_sms_size = 0x00; // SIZE
  this_sms->header.tlv_version = 0x01; // 3GPP

  this_sms->data.tlv = 0x06;
  // SMSC NUMBER SIZE RAW, we leave it hardcoded
  /* We shouldn't need to worry too much about the SMSC
   * since we're not actually sending this but...
   */
   /* SMSC */
  this_sms->data.smsc.phone_number_size = 0x07; //hardcoded as we use a dummy one
  this_sms->data.smsc.is_international_number = 0x91; // yes
  this_sms->data.smsc.number[0] = 0x00;
  this_sms->data.smsc.number[1] = 0x00;
  this_sms->data.smsc.number[2] = 0x00;
  this_sms->data.smsc.number[3] = 0x00;
  this_sms->data.smsc.number[4] = 0x00;
  this_sms->data.smsc.number[5] = 0xf0;

  this_sms->data.unknown = 0x04; // This is still unknown

  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   * 4 bits for each number, backwards  */
   /* PHONE NUMBER */
  this_sms->data.phone.phone_number_size = 0x0b; // hardcoded
  this_sms->data.phone.is_international_number = 0x91; // yes
  this_sms->data.phone.number[0] = 0x51;
  this_sms->data.phone.number[1] = 0x55;
  this_sms->data.phone.number[2] = 0x10;
  this_sms->data.phone.number[3] = 0x99;
  this_sms->data.phone.number[4] = 0x99;
  this_sms->data.phone.number[5] = 0xf9;
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
  this_sms->data.date.month =  swap_byte(tm.tm_mon + 1);
  this_sms->data.date.day =  swap_byte(tm.tm_mday);
  this_sms->data.date.hour =  swap_byte(tm.tm_hour);
  this_sms->data.date.minute =  swap_byte(tm.tm_min);
  this_sms->data.date.second =  swap_byte(tm.tm_sec);

  /* CONTENTS */
  this_sms->data.contents.content_tlv = 0x40;
  memcpy(this_sms->data.contents.contents, msgoutput, ret);

  /* SIZES AND LENGTHS */

  // Total packet size to send
  fullpktsz = sizeof(struct qmux_packet) + 
              sizeof(struct qmi_packet) +
              sizeof(struct qmi_generic_result_ind) +
              sizeof(struct wms_raw_message_header) + 
              sizeof(struct wms_user_data) -
              MAX_MESSAGE_SIZE + 
              ret; // ret == msgsize
  // QMUX packet size
  this_sms->qmuxpkt.packet_length = fullpktsz - sizeof(uint8_t); // ret == msgsize, last uint qmux ctlid 
  // QMI SZ: Full packet - QMUX header
  this_sms->qmipkt.length = sizeof(struct qmi_generic_result_ind) +
                            sizeof(struct wms_raw_message_header) + 
                            sizeof(struct wms_user_data) -
                            MAX_MESSAGE_SIZE + 
                            ret;
  // Header size: QMI - indication size - uint16_t size element itself - header tlv
  this_sms->header.size = this_sms->qmipkt.length - 
                            sizeof(struct qmi_generic_result_ind) - 
                            (3* sizeof(uint8_t));
  // User size: QMI - indication - header - uint16_t size element - own tlv
  this_sms->data.user_data_size = this_sms->qmipkt.length - 
                          sizeof(struct qmi_generic_result_ind) - 
                          sizeof(struct wms_raw_message_header) -
                          (3* sizeof(uint8_t));

  /* Content size is the number of bytes _after_ conversion
   * from GSM7 to ASCII bytes (not the actual size of string)
   */
  this_sms->data.contents.content_sz = strlen((char *)msg);

  ret = write(fd, this_sms, fullpktsz);
  dump_pkt_raw((uint8_t *)this_sms, fullpktsz);
  free(this_sms);
  this_sms = NULL;
  sms_runtime.curr_transaction_id++;
  return 0;
}

uint8_t do_inject_notification(int fd) {
  set_notif_pending(false);
  set_pending_notification_source(MSG_NONE);
  generate_message_notification(fd, 0);
  return 0;
}

/*  QMI device should be the USB socket here, we are talking
 *  in private with out host, ADSP doesn't need to know
 *  anything about this
 *  This func does the entire transaction
 */
uint8_t do_inject_custom_message(int fd, uint8_t *message) {
  int ret, pret;
  fd_set readfds;
  struct timeval tv;
  uint8_t tmpbuf[512];

  /*
   * 1. Send new message notification
   * 2. Wait for answer from the Pinephone for a second (retry if no answer)
   * 3. Send message to pinephone
   * 4. Wait 2 ack events
   * 5. Respond 2 acks
   */
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  set_notif_pending(false);
  set_pending_notification_source(MSG_NONE);
  generate_message_notification(fd, 0);
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, &tmpbuf, 512);
    if (ret > 7) {
      sms_runtime.curr_transaction_id = tmpbuf[7];
      ret = build_and_send_sms(fd, message);
    }
  }

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, &tmpbuf, 512);
    if (ret > 7) {
      sms_runtime.curr_transaction_id = tmpbuf[7];
      process_message_deletion(fd, 0, 0);
    } else {
      logger(MSG_DEBUG, "%s: Message too short\n", __func__);
    }
  } else {
    logger(MSG_DEBUG, "%s: No ACK requested back (0)\n", __func__);

  }

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, &tmpbuf, 512);
    if (ret > 7) {
      sms_runtime.curr_transaction_id = tmpbuf[7];
      process_message_deletion(fd, 0, 1);
    }
  } else {
      logger(MSG_DEBUG, "%s: No ACK requested back (1)\n", __func__);
  }

  return 0;
}
uint8_t inject_message(int fd, uint8_t message_id) {
  return do_inject_custom_message(fd, (uint8_t *)"Hello world!");
}

uint8_t send_outgoing_msg_ack(uint8_t transaction_id, uint8_t usbfd) {
  int ret;
  struct sms_received_ack *receive_ack;
  receive_ack = calloc(1, sizeof(struct sms_received_ack));
  receive_ack->qmuxpkt.version = 0x01;
  receive_ack->qmuxpkt.packet_length = 0x0018; // SIZE
  receive_ack->qmuxpkt.control = 0x80;
  receive_ack->qmuxpkt.service = 0x05;
  receive_ack->qmuxpkt.instance_id = 0x01;

  receive_ack->qmipkt.ctlid = 0x0002;
  receive_ack->qmipkt.transaction_id = transaction_id;
  receive_ack->qmipkt.msgid = WMS_RAW_SEND;
  receive_ack->qmipkt.length = 0x000c; // SIZE
  receive_ack->indication.result_code_type = TLV_QMI_RESULT;
  receive_ack->indication.generic_result_size = 0x04;
  receive_ack->indication.result = 0x00;
  receive_ack->indication.response = 0x00;

  receive_ack->user_data_tlv = 0x01;
  receive_ack->user_data_length = 0x0002;
  receive_ack->user_data_value = 0x0021;
  ret = write(usbfd, receive_ack, sizeof(struct sms_received_ack));
  free(receive_ack);
  return ret;
}

/* Intercept and ACK a message */
uint8_t intercept_and_parse(void *bytes, size_t len, uint8_t adspfd,
                            uint8_t usbfd) {
  size_t temp_sz;
  uint8_t *output, *reply;
  uint8_t ret;
  int outsize;
  struct outgoing_sms_packet *pkt;
  struct outgoing_no_date_sms_packet *nodate_pkt;

  output = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));

  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    nodate_pkt = (struct outgoing_no_date_sms_packet *)bytes;
    if (pkt->padded_tlv == 0x31) {
      ret = gsm7_to_ascii(pkt->contents.contents,
                          strlen((char *)pkt->contents.contents),
                          (char *)output, pkt->contents.content_sz);
    } else if (pkt->padded_tlv == 0x01) {
      ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                          strlen((char *)nodate_pkt->contents.contents),
                          (char *)output, nodate_pkt->contents.content_sz);
    }

    send_outgoing_msg_ack(pkt->qmipkt.transaction_id, usbfd);
    if (strstr((char *)output, "repeat me:") != 0) {
      do_inject_custom_message(usbfd, output);
    } else {
      parse_command(output, reply);
      do_inject_custom_message(usbfd, reply);
    }
  }

  free(output);
  return 0;
}



/* Intercept and ACK a message */
uint8_t decode(void *bytes, size_t len, uint8_t adspfd,
                            uint8_t usbfd) {
  size_t temp_sz;
  uint8_t *output, *reply;
  uint8_t ret;
  int outsize;
  struct outgoing_sms_packet *pkt;
  struct outgoing_no_date_sms_packet *nodate_pkt;

  output = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));
  reply = calloc(MAX_MESSAGE_SIZE, sizeof(uint8_t));

  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    nodate_pkt = (struct outgoing_no_date_sms_packet *)bytes;
    if (pkt->padded_tlv == 0x31) {
      ret = gsm7_to_ascii(pkt->contents.contents,
                          strlen((char *)pkt->contents.contents),
                          (char *)output, pkt->contents.content_sz);
    } else if (pkt->padded_tlv == 0x01) {
      ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                          strlen((char *)nodate_pkt->contents.contents),
                          (char *)output, nodate_pkt->contents.content_sz);
    }

    send_outgoing_msg_ack(pkt->qmipkt.transaction_id, usbfd);
    if (strstr((char *)output, "repeat me:") != 0) {
      do_inject_custom_message(usbfd, output);
    } else {
      parse_command(output, reply);
      do_inject_custom_message(usbfd, reply);
    }
  }

  free(output);
  return 0;
}