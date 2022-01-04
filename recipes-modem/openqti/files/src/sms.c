// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

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
  uint16_t curr_transaction_id;
  uint8_t demo_text_no;
} sms_runtime;

void reset_sms_runtime() { 
  sms_runtime.notif_pending = false; 
  sms_runtime.curr_transaction_id = 7; 
  sms_runtime.demo_text_no = 0;
  }

void set_notif_pending(bool en) { sms_runtime.notif_pending = en; }
bool is_message_pending() {
  if (sms_runtime.notif_pending)
   logger(MSG_INFO, "%s: Is message pending? %i \n", __func__,
         sms_runtime.notif_pending);
  return sms_runtime.notif_pending;
}

uint8_t get_data(char input, uint8_t *output) {
  if (input - '0' >= 0 && '9' - input >= 0) {
    *output = input - '0';
  } else if (input - 'a' >= 0 && 'f' - input >= 0) {
    *output = input - 'a' + 10;
  } else if (input - 'A' >= 0 && 'F' - input >= 0) {
    *output = input - 'A' + 10;
  } else {
    return 1;
  }

  return 0;
}

uint8_t get_data_pair(const char *input, uint8_t *output) {
  uint8_t data;

  if (get_data(*input, &data) != 0) {
    return 1;
  }

  *output = data << 4;
  if (get_data(*(input + 1), &data) != 0) {
    return 2;
  }

  *output = *output | data;

  return 0;
}
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
  logger(MSG_INFO, "%s: Return %i", __func__, ret);
  dump_pkt_raw((uint8_t *)notif_pkt, sizeof(struct sms_notif_packet));
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
    logger(MSG_INFO, "%s: Send message %i\n", __func__, pending_message_num);
    ctl_pkt->unk1 = 0x02;
    ctl_pkt->unk2 = 0x04;
    ctl_pkt->unk3 = 0x00;
    ctl_pkt->unk4 = 0x00;
    ctl_pkt->unk5 = 0x00;
    ctl_pkt->unk6 = 0x00;
    ctl_pkt->unk7 = 0x00;
    ret = write(fd, ctl_pkt, sizeof(struct sms_control_packet));
  } else {
    logger(MSG_INFO, "%s: Send message %i\n", __func__, pending_message_num);
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

uint8_t build_message(uint8_t type, uint8_t method, char *origin, char *smsc,
                      char *contents) {

  return 0;
}

uint8_t parse_message_data(char *buffer) { 
  
  return 0; 
}


uint8_t ascii_to_gsm7(const char* in, uint8_t* out) {
  unsigned bit_count = 0;
  unsigned bit_queue = 0;
  uint8_t bytes_written = 0;
  while (*in) {
    bit_queue |= (*in & 0x7Fu) << bit_count;
    bit_count += 7;
    if (bit_count >= 8) {
      *out++ = (uint8_t) bit_queue;
      bytes_written++;
      bit_count -= 8;
      bit_queue >>= 8;
    }
    in++;
  }
  if (bit_count > 0) {
    *out++ = (uint8_t) bit_queue;
    bytes_written++;
    }

  return bytes_written;
}

/*
 * Actually build and inject the message
 * Sizes will have to be calculated when everything is set
 */
int build_and_send_sms(int fd, char *msg) {
  struct sms_packet *this_sms;
  this_sms = calloc(1, sizeof(struct sms_packet));
  int ret, fullpktsz, internal_pktsz;
  uint8_t demotext[] = {0xd4, 0xe2, 0x94, 0xea, 0x0a, 0x0a, 0x87, 0xc4, 0xa2, 0xf1, 0x88, 0x4c, 0x2a, 0x97, 0x4c};
  uint8_t msgoutput[140] = {0};

  ret = ascii_to_gsm7(msg, msgoutput);
  logger(MSG_INFO, "%s: Bytes to write %i\n", __func__, ret);
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

  this_sms->incoming_smsc.sz = 0x07; // SMSC NUMBER SIZE RAW, we live it hardcoded
  /* We shouldn't need to worry too much about the SMSC
   * since we're not actually sending this but...
   */
  this_sms->incoming_smsc.smsc_number[0] = 0x91;
  this_sms->incoming_smsc.smsc_number[1] = 0x10;
  this_sms->incoming_smsc.smsc_number[2] = 0x01;
  this_sms->incoming_smsc.smsc_number[3] = 0x10;
  this_sms->incoming_smsc.smsc_number[4] = 0x01;
  this_sms->incoming_smsc.smsc_number[5] = 0x00;
  this_sms->incoming_smsc.smsc_number[6] = 0xf0;
  this_sms->caller.unknown = 0x04; // Sample
  this_sms->caller.sz = 0x0b; 
  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   */
  this_sms->caller.phone_number[0] = 0x91;
  this_sms->caller.phone_number[1] = 0x10;
  this_sms->caller.phone_number[2] = 0x01;
  this_sms->caller.phone_number[3] = 0x10;
  this_sms->caller.phone_number[4] = 0x01;
  this_sms->caller.phone_number[5] = 0x00;
  this_sms->caller.phone_number[6] = 0xf0;
  this_sms->meta.unknown = 0x00; // 0x00 0x00
  /* 4 bits for each number, backwards
   * 22 / 01 / 01 06:31:12
   * 0x40 at the end
   */
  this_sms->meta.year = 0x22; // 0x12 0x22 0x32
  this_sms->meta.month = 0x10;
  this_sms->meta.day = 0x10;
  this_sms->meta.hour = 0x60;
  this_sms->meta.minute = 0x13;
  this_sms->meta.second = 0x12;
  this_sms->meta.unknown2 = 0x40;
  /* Content size is the number of bytes _after_ conversion
   * from GSM7 to ASCII bytes (not the actual size of string)
   */
  this_sms->contents.content_sz = 0x11;
  // Copy the parsed string
  memcpy(this_sms->contents.contents, msgoutput, ret);
  fullpktsz = sizeof (struct qmux_packet) +  sizeof (struct qmi_packet) +  sizeof (struct unknown_interim_data)+
               sizeof (struct sms_pkt_header) +  sizeof (struct smsc_data) +  sizeof (struct sms_caller_data) +
                sizeof (struct sms_metadata) + sizeof(uint8_t) + ret;
  
  internal_pktsz = sizeof (struct qmux_packet) +  sizeof (struct qmi_packet) +  sizeof (struct unknown_interim_data)+
               sizeof (struct sms_pkt_header) +  sizeof (struct smsc_data) +  sizeof (struct sms_caller_data) +
                sizeof (struct sms_metadata) + ret;
  // sizeof(struct sms_packet) - ((MAX_MESSAGE_SIZE/2) + ret);
  // Now calculate packet lengths at different points
  this_sms->qmuxpkt.packet_length = internal_pktsz; // SIZE
  this_sms->qmipkt.length = internal_pktsz - sizeof(struct qmux_packet) - sizeof(struct qmi_packet) + sizeof(uint8_t); // SIZE
  this_sms->unknown_data.wms_sms_size = this_sms->qmipkt.length - sizeof (struct unknown_interim_data);
  this_sms->header.sms_content_sz = this_sms->unknown_data.wms_sms_size - sizeof(struct sms_pkt_header);
  this_sms->contents.content_sz = strlen(msg); // 0x11; // Size after converting to gsm7
//  this_sms->contents.contents = {0xd4, 0xe2, 0x94, 0xea, 0x0a, 0x0a, 0x87, 0xc4, 0xa2, 0xf1, 0x88, 0x4c, 0x2a, 0x97, 0x4c};
 
  ret = write(fd, this_sms, fullpktsz);
  dump_pkt_raw((uint8_t *)this_sms, fullpktsz);
  free(this_sms);
  this_sms = NULL;
  sms_runtime.curr_transaction_id++;
  return 0;
}

/*  QMI device should be the USB socket here, we are talking
 *  in private with out host, ADSP doesn't need to know
 *  anything about this
 *  This func does the entire transaction
 */
uint8_t do_inject_message(int fd, uint8_t message_id) {
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
   *
   */
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  set_notif_pending(false);
  logger(MSG_INFO, "%s: Generating notif\n", __func__);
  generate_message_notification(fd, 0);
  logger(MSG_INFO, "%s: ACK from Pinephone?\n", __func__);
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    logger(MSG_INFO, "%s: Seems we got an ACK?? Let's get it\n", __func__);
    ret = read(fd, &tmpbuf, 512);
    logger(MSG_INFO, "%s: Got %i bytes", __func__, ret);
    dump_pkt_raw(tmpbuf, ret);
    sms_runtime.curr_transaction_id = tmpbuf[7];
  }
  logger(MSG_INFO, "%s Even if we didnt, send it!\n", __func__);
  ret  = build_and_send_sms(fd, sample_text[message_id].text);

  logger(MSG_INFO, "%s: Return %i\n NOW READ AGAIN FOR AN ACK", __func__, ret);

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    logger(MSG_INFO, "%s: First message\n", __func__);
    ret = read(fd, &tmpbuf, 512);

    sms_runtime.curr_transaction_id = tmpbuf[7];

    logger(MSG_INFO, "%s: Got %i bytes", __func__, ret);
    dump_pkt_raw(tmpbuf, ret);
  }
  ack_message_notification_request(fd, 0);
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    logger(MSG_INFO, "%s: Second message\n", __func__);
    ret = read(fd, &tmpbuf, 512);

        sms_runtime.curr_transaction_id = tmpbuf[7];

    logger(MSG_INFO, "%s: Got %i bytes", __func__, ret);
    dump_pkt_raw(tmpbuf, ret);
  }
  ack_message_notification_request(fd, 1);

  logger(MSG_INFO, "%s: END OF INJECT\n", __func__);

  return 0;
}

uint8_t inject_message(int fd, uint8_t message_id) {
if (sms_runtime.demo_text_no > 5) {
  sms_runtime.demo_text_no = 0;
} 
do_inject_message(fd, sms_runtime.demo_text_no);
sms_runtime.demo_text_no++;
return 0;

}