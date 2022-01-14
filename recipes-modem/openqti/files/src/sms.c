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

/*
 * This sends a notification message, ModemManager should answer it
 * with a request to get the message
 */
uint8_t generate_message_notification(int fd, uint8_t pending_message_num) {
  int ret;
  struct sms_notif_packet *notif_pkt;
  notif_pkt = calloc(1, sizeof(struct sms_notif_packet));
  notif_pkt->qmuxpkt.version = 0x01;
  notif_pkt->qmuxpkt.packet_length = 0x1c; // SIZE
  notif_pkt->qmuxpkt.control = 0x80;
  notif_pkt->qmuxpkt.service = 0x05;
  notif_pkt->qmuxpkt.instance_id = 0x01;

  notif_pkt->qmipkt.ctlid = 0x04;
  notif_pkt->qmipkt.transaction_id = 0x0002;
  notif_pkt->qmipkt.msgid = MSG_NOTIFICATION;
  notif_pkt->qmipkt.length = 0x10; // SIZE

  notif_pkt->size = 0x10;
  notif_pkt->service = htole16(5);
  notif_pkt->instance = htole16(1);
  notif_pkt->unkn4 = 0x00;
  notif_pkt->unkn5 = 0x00;
  notif_pkt->unkn6 = 0x00;
  notif_pkt->unkn7 = 0x12;
  notif_pkt->unkn8 = 0x01;
  notif_pkt->unkn9 = 0x00;
  notif_pkt->unkn10 = 0x01;
  notif_pkt->unkn11 = 0x16;
  notif_pkt->unkn12 = 0x01;
  notif_pkt->unkn13 = 0x00;
  notif_pkt->unkn14 = 0x00;

  ret = write(fd, notif_pkt, sizeof(struct sms_notif_packet));
  sms_runtime.curr_transaction_id++;

  free(notif_pkt);
  notif_pkt = NULL;
  return 0;
}

/*
 * After sending the message, we need to answer to 2 more QMI messages
 *
 */
uint8_t ack_message_notification_request(int fd, uint8_t pending_message_num) {
  int ret;
  struct sms_control_packet *ctl_pkt;
  ctl_pkt = calloc(1, sizeof(struct sms_control_packet));
  ctl_pkt->qmuxpkt.version = 0x01;
  ctl_pkt->qmuxpkt.packet_length = 0x13; // SIZE
  ctl_pkt->qmuxpkt.control = 0x80;
  ctl_pkt->qmuxpkt.service = 0x05;
  ctl_pkt->qmuxpkt.instance_id = 0x01;

  ctl_pkt->qmipkt.ctlid = 0x02;
  ctl_pkt->qmipkt.transaction_id = sms_runtime.curr_transaction_id;
  ctl_pkt->qmipkt.msgid = MSG_CONTROL_ACK;
  ctl_pkt->qmipkt.length = 0x07; // SIZE

  if (pending_message_num) {
    logger(MSG_DEBUG, "%s: Send message %i\n", __func__, pending_message_num);
    ctl_pkt->unk1 = 0x02;
    ctl_pkt->unk2 = 0x04;
    ctl_pkt->unk3 = 0x00;
    ctl_pkt->unk4 = 0x00;
    ctl_pkt->unk5 = 0x00;
    ctl_pkt->unk6 = 0x00;
    ctl_pkt->unk7 = 0x00;
    ret = write(fd, ctl_pkt, sizeof(struct sms_control_packet));
  } else {
    logger(MSG_DEBUG, "%s: Send message %i\n", __func__, pending_message_num);
    ctl_pkt->unk1 = 0x02;
    ctl_pkt->unk2 = 0x04;
    ctl_pkt->unk3 = 0x00;
    ctl_pkt->unk4 = 0x01;
    ctl_pkt->unk5 = 0x00;
    ctl_pkt->unk6 = 0x32;
    ctl_pkt->unk7 = 0x00;
    ret = write(fd, ctl_pkt, sizeof(struct sms_control_packet));
  }
  sms_runtime.curr_transaction_id++;
  free(ctl_pkt);
  ctl_pkt = NULL;
  return 0;
}

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

/*
 * Actually build and inject the message
 * Sizes will have to be calculated when everything is set
 */
int build_and_send_sms(int fd, uint8_t *msg) {
  struct incoming_sms_packet *this_sms;
  this_sms = calloc(1, sizeof(struct incoming_sms_packet));
  int ret, fullpktsz, internal_pktsz;
  int tmpyear;
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  uint8_t msgoutput[140] = {0};
  ret = ascii_to_gsm7(msg, msgoutput);
  logger(MSG_DEBUG, "%s: Bytes to write %i\n", __func__, ret);
  this_sms->qmuxpkt.version = 0x01;
  this_sms->qmuxpkt.packet_length = 0x00; // SIZE
  this_sms->qmuxpkt.control = 0x80;
  this_sms->qmuxpkt.service = 0x05;
  this_sms->qmuxpkt.instance_id = 0x01;

  this_sms->qmipkt.ctlid = 0x0002;
  this_sms->qmipkt.transaction_id = sms_runtime.curr_transaction_id;
  this_sms->qmipkt.msgid = MSG_INDICATION;
  this_sms->qmipkt.length = 0x00; // SIZE

  this_sms->unknown_data.unk1 = 0x02;
  this_sms->unknown_data.unk2 = 0x04;
  this_sms->unknown_data.unk3 = 0x00;
  this_sms->unknown_data.unk4 = 0x00;
  this_sms->unknown_data.unk5 = 0x00;
  this_sms->unknown_data.unk6 = 0x00;
  this_sms->unknown_data.unk7 = 0x00;
  this_sms->unknown_data.unk8 = 0x01;
  this_sms->unknown_data.wms_sms_size = 0x00; // SIZE
  this_sms->header.unk9 = 0x01;
  this_sms->header.unk10 = 0x06;

  this_sms->incoming_smsc.sz =
      0x07; // SMSC NUMBER SIZE RAW, we live it hardcoded
  /* We shouldn't need to worry too much about the SMSC
   * since we're not actually sending this but...
   */
  this_sms->incoming_smsc.smsc_number[0] = 0x91;
  this_sms->incoming_smsc.smsc_number[1] = 0x00;
  this_sms->incoming_smsc.smsc_number[2] = 0x00;
  this_sms->incoming_smsc.smsc_number[3] = 0x00;
  this_sms->incoming_smsc.smsc_number[4] = 0x00;
  this_sms->incoming_smsc.smsc_number[5] = 0x00;
  this_sms->incoming_smsc.smsc_number[6] = 0xf0;
  this_sms->caller.unknown = 0x04; // Sample
  this_sms->caller.sz = 0x0b;
  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   */
  this_sms->caller.phone_number[0] = 0x91;
  this_sms->caller.phone_number[1] = 0x51;
  this_sms->caller.phone_number[2] = 0x55;
  this_sms->caller.phone_number[3] = 0x10;
  this_sms->caller.phone_number[4] = 0x99;
  this_sms->caller.phone_number[5] = 0x99;
  this_sms->caller.phone_number[6] = 0xf9;
  this_sms->meta.unknown = 0x00; // 0x00 0x00
  /* 4 bits for each number, backwards
   * 22 / 01 / 01 06:31:12
   * 0x40 at the end
   */

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
  this_sms->meta.year = (tmpyear << 4) | (tmpyear >> 4);
  this_sms->meta.month = ((tm.tm_mon + 1) << 4) | ((tm.tm_mon + 1) >> 4);
  this_sms->meta.day = (tm.tm_mday << 4) | (tm.tm_mday >> 4);
  this_sms->meta.hour = (tm.tm_hour << 4) | (tm.tm_hour >> 4);
  this_sms->meta.minute = (tm.tm_min << 4) | (tm.tm_min >> 4);
  this_sms->meta.second = (tm.tm_sec << 4) | (tm.tm_sec >> 4);
  this_sms->meta.unknown2 = 0x40;

  // Copy the parsed string
  memcpy(this_sms->contents.contents, msgoutput, ret);
  fullpktsz = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
              sizeof(struct unknown_interim_data) +
              sizeof(struct sms_incoming_header) + sizeof(struct smsc_data) +
              sizeof(struct sms_caller_data) + sizeof(struct sms_metadata) +
              sizeof(uint8_t) + ret;

  internal_pktsz = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   sizeof(struct unknown_interim_data) +
                   sizeof(struct sms_incoming_header) +
                   sizeof(struct smsc_data) + sizeof(struct sms_caller_data) +
                   sizeof(struct sms_metadata) + ret;
  // sizeof(struct sms_packet) - ((MAX_MESSAGE_SIZE/2) + ret);
  // Now calculate packet lengths at different points
  this_sms->qmuxpkt.packet_length = internal_pktsz; // SIZE
  this_sms->qmipkt.length = internal_pktsz - sizeof(struct qmux_packet) -
                            sizeof(struct qmi_packet) + sizeof(uint8_t); // SIZE
  this_sms->unknown_data.wms_sms_size =
      this_sms->qmipkt.length - sizeof(struct unknown_interim_data);
  this_sms->header.sms_content_sz =
      this_sms->unknown_data.wms_sms_size - sizeof(struct sms_incoming_header);
  /* Content size is the number of bytes _after_ conversion
   * from GSM7 to ASCII bytes (not the actual size of string)
   */
  this_sms->contents.content_sz = strlen((char *)msg);

  ret = write(fd, this_sms, fullpktsz);
 // dump_pkt_raw((uint8_t *)this_sms, fullpktsz);
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
      ack_message_notification_request(fd, 0);
    }
  }

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, &tmpbuf, 512);
    if (ret > 7) {
      sms_runtime.curr_transaction_id = tmpbuf[7];
      ack_message_notification_request(fd, 1);
    }
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
  receive_ack->qmipkt.msgid = MSG_ACK;
  receive_ack->qmipkt.length = 0x000c; // SIZE
  receive_ack->unk1 = 0x02;
  receive_ack->unk2 = 0x04;
  receive_ack->unk3 = 0x00;
  receive_ack->unk4 = 0x00;
  receive_ack->unk5 = 0x00;
  receive_ack->unk6 = 0x01;
  receive_ack->unk7 = 0x0002;
  receive_ack->unk8 = 0x0021;
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

  output = calloc(256, sizeof(uint8_t));
  reply = calloc(256, sizeof(uint8_t));

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