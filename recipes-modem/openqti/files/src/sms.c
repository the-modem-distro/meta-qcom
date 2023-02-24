// SPDX-License-Identifier: MIT

#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "atfwd.h"
#include "call.h"
#include "cell_broadcast.h"
#include "command.h"
#include "config.h"
#include "helpers.h"
#include "ipc.h"
#include "logger.h"
#include "proxy.h"
#include "qmi.h"
#include "sms.h"
#include "timesync.h"

/* Workaround while I debug pdu_decode()*/
#define MSG_PDU_DECODE MSG_DEBUG
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
  uint8_t is_raw;
  uint8_t is_cb;
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
  uint32_t pending_messages_from_adsp;
  struct message_queue queue;
  uint8_t *stuck_message_data;
  bool stuck_message_data_pending;
} sms_runtime;

void reset_sms_runtime() {
  sms_runtime.notif_pending = false;
  sms_runtime.curr_transaction_id = 0;
  sms_runtime.source = -1;
  sms_runtime.queue.queue_pos = -1;
  sms_runtime.current_message_id = 0;
  sms_runtime.pending_messages_from_adsp = 0;
  sms_runtime.stuck_message_data = NULL;
  sms_runtime.stuck_message_data_pending = false;
}

void set_notif_pending(bool pending) { sms_runtime.notif_pending = pending; }

void set_pending_notification_source(uint8_t source) {
  sms_runtime.source = source;
}

uint8_t get_notification_source() { return sms_runtime.source; }

uint32_t get_pending_messages_in_adsp() {
  return sms_runtime.pending_messages_from_adsp;
}

bool is_stuck_message_retrieve_pending() {
  return sms_runtime.stuck_message_data_pending;
}

void set_pending_messages_in_adsp(uint32_t index) {
  if (index > 0 && sms_runtime.pending_messages_from_adsp > 0 &&
      (index - sms_runtime.pending_messages_from_adsp) > 1) {
    logger(MSG_WARN,
           "WARNING, Pending messages now is: %u and it was %u before!",
           index + 1, sms_runtime.pending_messages_from_adsp);
  }
  logger(MSG_INFO, "Pending ADSP Messages: %u", index + 1);
  sms_runtime.pending_messages_from_adsp = index;
}

uint32_t get_internal_pending_messages() {
  return sms_runtime.current_message_id;
}

bool is_message_pending() { return sms_runtime.notif_pending; }

/* 2022-12-20 | Migrating to modemmanager code */
int gsm7_to_ascii(const unsigned char *buffer, int buffer_length,
                  char *output_sms_text, int sms_text_length,
                  uint8_t bit_offset) {

  int bytes_out = 0;
  int i;

  for (i = 0; i < buffer_length; i++) {
    uint8_t bits_here, bits_in_next, octet, offset, c;
    uint32_t start_bit;

    start_bit = bit_offset + (i * 7); /* Overall bit offset of char in buffer */
    offset = start_bit % 8;           /* Offset to start of char in this byte */
    bits_here = offset ? (8 - offset) : 7;
    bits_in_next = 7 - bits_here;

    /* Grab bits in the current byte */
    octet = buffer[start_bit / 8];
    c = (octet >> offset) & (0xFF >> (8 - bits_here));

    /* Grab any bits that spilled over to next byte */
    if (bits_in_next) {
      octet = buffer[(start_bit / 8) + 1];
      c |= (octet & (0xFF >> (8 - bits_in_next))) << bits_here;
    }
    output_sms_text[bytes_out] = c;
    bytes_out++;
    // g_byte_array_append (unpacked, &c, 1);
  }
  return bytes_out;
}

int decode_number(uint8_t *data, uint8_t len, char *number) {

  uint8_t count = 0;
  uint8_t num_type = data[0] & SMS_NUMBER_TYPE_MASK;

  if (len < 1) {
    logger(MSG_PDU_DECODE, "Invalid length: %u", len);
    number[0] = 'E';
    return -EINVAL;
  }
  if (num_type == SMS_NUMBER_TYPE_ALPHA) {
    logger(MSG_PDU_DECODE, "(text) ");
    gsm7_to_ascii(data + 1, len - 1, number, len * 2, 0);
  } else {
    logger(MSG_PDU_DECODE, "(number) %i --> ", len);
    if (data[0] == 0x91) {
      number[count] = '+';
      count++;
    }
    for (uint8_t i = count; i < len; i++) {
      printf("|%.2x  |", data[i]);
      number[count] = (data[i] & 0x0f) + '0';
      number[count + 1] = ((data[i] & 0xf0) >> 4) + '0';
      count += 2;
      if (data[i] > 0xf0) {
        number[count - 1] = '\0';
        break;
      }
    }
  }
  return 0;
}

int sms_decode_timestamp(uint8_t *timestamp, char *output) {
  /* ISO8601 format: YYYY-MM-DDTHH:MM:SS+HHMM */
  uint16_t year, month, day, hour, minute, second;
  uint16_t quarters, offset_minutes;

  year = 2000 + ((timestamp[0] & 0xf) * 10) + ((timestamp[0] >> 4) & 0xf);
  month = ((timestamp[1] & 0xf) * 10) + ((timestamp[1] >> 4) & 0xf);
  day = ((timestamp[2] & 0xf) * 10) + ((timestamp[2] >> 4) & 0xf);
  hour = ((timestamp[3] & 0xf) * 10) + ((timestamp[3] >> 4) & 0xf);
  minute = ((timestamp[4] & 0xf) * 10) + ((timestamp[4] >> 4) & 0xf);
  second = ((timestamp[5] & 0xf) * 10) + ((timestamp[5] >> 4) & 0xf);
  quarters = ((timestamp[6] & 0x7) * 10) + ((timestamp[6] >> 4) & 0xf);
  offset_minutes = quarters * 15;
  if (timestamp[6] & 0x08)
    offset_minutes = -1 * offset_minutes;

  snprintf(output, 128, "%.4u-%.2u-%.2u %.2u:%.2u:%.2u", year, month, day, hour,
           minute, second);
  return 0;
}

uint8_t sms_encoding_type(int dcs) {
  uint8_t scheme = MM_SMS_ENCODING_UNKNOWN;

  switch ((dcs >> 4) & 0xf) {
  /* General data coding group */
  case 0:
  case 1:
  case 2:
  case 3:
    switch (dcs & 0x0c) {
    case 0x08:
      scheme = MM_SMS_ENCODING_UCS2;
      break;
    case 0x00:
      /* reserved - spec says to treat it as default alphabet */
      /* Fall through */
    case 0x0c:
      scheme = MM_SMS_ENCODING_GSM7;
      break;
    case 0x04:
      scheme = MM_SMS_ENCODING_8BIT;
      break;
    default:
      logger(MSG_PDU_DECODE, "ERROR GETTING DCS");
    }
    break;

    /* Message waiting group (default alphabet) */
  case 0xc:
  case 0xd:
    scheme = MM_SMS_ENCODING_GSM7;
    break;

    /* Message waiting group (UCS2 alphabet) */
  case 0xe:
    scheme = MM_SMS_ENCODING_UCS2;
    break;

    /* Data coding/message class group */
  case 0xf:
    switch (dcs & 0x04) {
    case 0x00:
      scheme = MM_SMS_ENCODING_GSM7;
      break;
    case 0x04:
      scheme = MM_SMS_ENCODING_8BIT;
      break;
    default:
      logger(MSG_PDU_DECODE, "ERROR GETTING DCS2");
    }
    break;

    /* Reserved coding group values - spec says to treat it as default alphabet
     */
  default:
    scheme = MM_SMS_ENCODING_GSM7;
    break;
  }

  return scheme;
}

int pdu_decode(uint8_t *buffer, size_t size, struct message_data *msg_out, uint8_t *out_dcs) {

  struct incoming_message *msg;
  uint8_t offset = 0;

  uint8_t validity_format = 0;
  /* PID & DCS */
  uint8_t tp_pid_offset = 0;
  uint8_t tp_dcs_offset = 0;

  /* User data length */
  uint8_t tp_user_data_len_offset = 0;
  uint8_t user_data_encoding = 0;
  uint8_t has_udh = 0;

  logger(MSG_PDU_DECODE, "%s: Attempting to decode PDU from %i byte message",
         __func__, size);

  msg = (struct incoming_message *)buffer;
  logger(MSG_PDU_DECODE, "- Message size: %.4x\n", msg->msg.raw_message_len);
  logger(MSG_PDU_DECODE, "- Storage type: %.2x\n", msg->msg.storage_type);
  logger(MSG_PDU_DECODE, "- Message format: %.2x\n", msg->msg.message_format);
  if (msg->msg.data[0] > 0) { // Has a SMSC
    logger(MSG_PDU_DECODE, "SMSC at offset: %u\n", offset);
    offset += msg->msg.data[0] + 1;
    decode_number(msg->msg.data + 1, msg->msg.data[0], msg_out->smsc);
    logger(MSG_PDU_DECODE, " - Message contains a SMSC: %s\n", msg_out->smsc);
  }

  uint8_t pdu_type = (msg->msg.data[offset] & SMS_TP_MTI_MASK);
  switch (pdu_type) {
  case SMS_TP_MTI_SMS_DELIVER:
    logger(MSG_PDU_DECODE, " - PDU Type: SMS_TP_MTI_SMS_DELIVER\n");
    break;
  case SMS_TP_MTI_SMS_SUBMIT:
    logger(MSG_PDU_DECODE, " - PDU Type: SMS_TP_MTI_SMS_SUBMIT\n");
    break;
  case SMS_TP_MTI_SMS_STATUS_REPORT:
    logger(MSG_PDU_DECODE, " - PDU Type: SMS_TP_MTI_SMS_STATUS_REPORT\n");
    break;
  default:
    logger(MSG_PDU_DECODE, "  - Unknown message type 0x%.2x!\n", pdu_type);
    return -1;
    break;
  }
  /* Delivery report was requested? */
  if (msg->msg.data[offset] & 0x20)
    logger(MSG_PDU_DECODE, "  - Delivery report requested\n");
  /* PDU with validity? (only in SUBMIT PDUs) */
  if (pdu_type == SMS_TP_MTI_SMS_SUBMIT) {
    logger(MSG_PDU_DECODE, "  - Validity format: %.2x\n",
           msg->msg.data[offset] & 0x18);
    validity_format = msg->msg.data[offset] & 0x18;
  }
  /* PDU with user data header? */
  if (msg->msg.data[offset] & 0x40) {
    logger(MSG_PDU_DECODE, "  - PDU has user data header\n");
    has_udh = 1;
  }

  offset++;

  /* TP-MR (1 byte, in STATUS_REPORT and SUBMIT PDUs */
  if (pdu_type == SMS_TP_MTI_SMS_STATUS_REPORT ||
      pdu_type == SMS_TP_MTI_SMS_SUBMIT) {
    if (size < offset + 1) {
      logger(MSG_ERROR, "%s: Packet too short!\n", __func__);
      return -ENOMEM;
    }
    logger(MSG_PDU_DECODE, "  - Message reference: %u\n",
           msg->msg.data[offset]);
    offset++;
  }

  /* TP-DA or TP-OA or TP-RA
   * First byte represents the number of DIGITS in the number.
   * Round the sender address length up to an even number of
   * semi-octets, and thus an integral number of octets.
   */

  uint8_t tp_addr_size_digits = msg->msg.data[offset++];
  uint8_t tp_addr_size_bytes = (tp_addr_size_digits + 1) >> 1;
  if (size < offset + tp_addr_size_bytes) {
    logger(MSG_ERROR, "%s: Packet too short (number)!\n", __func__);
    return -ENOMEM;
  }
  logger(MSG_PDU_DECODE, " - Phone number: ");
  decode_number(msg->msg.data + offset, tp_addr_size_digits,
                msg_out->phone_number);

  logger(MSG_PDU_DECODE, "%s\n", msg_out->phone_number);

  offset += (1 + tp_addr_size_bytes); /* +1 due to the Type of Address byte */
  /* PID, DCS and UDL/UD */
  if (pdu_type == SMS_TP_MTI_SMS_DELIVER) {
    logger(MSG_PDU_DECODE, " - Type: SMS_TP_MTI_SMS_DELIVER\n");
    char date[128];
    if (size < offset + 9) {
      logger(MSG_ERROR, "%s: Packet too short [PID/DCS/TIMESTAMP]!\n",
             __func__);
      return -ENOMEM;
    }

    /* ------ TP-PID (1 byte) ------ */
    tp_pid_offset = offset++;

    /* ------ TP-DCS (1 byte) ------ */
    tp_dcs_offset = offset++;

    /* ------ Timestamp (7 bytes) ------ */
    sms_decode_timestamp(msg->msg.data + offset, date);
    offset += 7;
    logger(MSG_PDU_DECODE, " - Date: %s\n", date);
    tp_user_data_len_offset = offset;

    logger(MSG_PDU_DECODE, "Current pos: %.2x %.2x\n", msg->msg.data[offset],
           msg->msg.data[offset + 1]);
  } else if (pdu_type == SMS_TP_MTI_SMS_SUBMIT) {
    logger(MSG_PDU_DECODE, " - Type: SMS_TP_MTI_SMS_SUBMIT\n");

    if (size < offset + 2 + !!validity_format) {
      logger(MSG_ERROR, "%s: Packet too short [validity]!\n", __func__);
      return -ENOMEM;
    }

    /* ------ TP-PID (1 byte) ------ */
    tp_pid_offset = offset++;
    /* ------ TP-DCS (1 byte) ------ */
    tp_dcs_offset = offset++;
    /* ----------- TP-Validity-Period (1 byte) ----------- */
    if (validity_format) {
      switch (validity_format) {
      case 0x10:
        logger(MSG_PDU_DECODE, " - Validity is set: Relative format\n");
        offset += 1;
        break;
      case 0x08:
        /* TODO: support enhanced format; GSM 03.40 */
        logger(MSG_PDU_DECODE, " - Validity is set: Enhanced format\n");
        /* 7 bytes for enhanced validity */
        offset += 7;
        break;
      case 0x18:
        /* TODO: support absolute format; GSM 03.40 */
        logger(MSG_PDU_DECODE, " - Validity is set: Absolute format\n");
        /* 7 bytes for absolute validity */
        offset += 7;
        break;
      default:
        /* Cannot happen as we AND with the 0x18 mask */
        logger(MSG_PDU_DECODE, " - UNKNOWN VALIDITY\n");
      }
    }

    tp_user_data_len_offset = offset;
  } else if (pdu_type == SMS_TP_MTI_SMS_STATUS_REPORT) {
    logger(MSG_PDU_DECODE, " - Type: SMS_TP_MTI_SMS_STATUS_REPORT\n");
    char date_received_in_smsc[128];
    char date_sent_by_smsc[128];

    /* We have 2 timestamps in status report PDUs:
     *  first, the timestamp for when the PDU was received in the SMSC
     *  second, the timestamp for when the PDU was forwarded by the SMSC
     */
    if (size < offset + 15) {
      logger(MSG_ERROR, "%s: Packet too short while reading timestamps\n",
             __func__);
      return -ENOMEM;
    }
    /* ------ Timestamp (7 bytes) ------ */
    sms_decode_timestamp(msg->msg.data + offset, date_received_in_smsc);
    offset += 7;

    /* ------ Discharge Timestamp (7 bytes) ------ */

    sms_decode_timestamp(msg->msg.data + offset, date_sent_by_smsc);
    offset += 7;
    logger(MSG_PDU_DECODE,
           "Dates:\n - Received by smsc: %s \n - Released by smsc: %s\n",
           date_received_in_smsc, date_sent_by_smsc);
    /* ----- TP-STATUS (1 byte) ------ */
    logger(MSG_PDU_DECODE, "Delivery state: %u (%.2x)\n", msg->msg.data[offset],
           msg->msg.data[offset]);
    offset++;

    /* ------ TP-PI (1 byte) OPTIONAL ------ */
    if (offset < msg->msg.raw_message_len) {
      logger(MSG_PDU_DECODE, "Protocol ID is set\n");
      uint8_t next_optional_field_offset = offset + 1;

      /* TP-PID? */
      if (msg->msg.data[offset] & 0x01)
        tp_pid_offset = next_optional_field_offset++;

      /* TP-DCS? */
      if (msg->msg.data[offset] & 0x02)
        tp_dcs_offset = next_optional_field_offset++;

      /* TP-UserData? */
      if (msg->msg.data[offset] & 0x04)
        tp_user_data_len_offset = next_optional_field_offset;
    }
  } else {
    logger(MSG_ERROR, "%s: Error when trying to find the message type!\n");
    return -EINVAL;
  }

  if (tp_pid_offset > 0) {
    if (size < tp_pid_offset + 1) {
      logger(MSG_ERROR, "%s: Packet too short reading Protocol ID\n", __func__);
      return -ENOMEM;
    }
    logger(MSG_PDU_DECODE, " - Protocol ID: %u \n",
           msg->msg.data[tp_pid_offset]);
  }

  /* Grab user data encoding and message class */
  if (tp_dcs_offset > 0) {
    if (size < tp_dcs_offset + 1) {
      logger(MSG_ERROR, "%s: Packet too short reading the data coding scheme\n",
             __func__);
      return -ENOMEM;
    }
    /* Encoding given in the 'alphabet' bits */
    user_data_encoding = sms_encoding_type(msg->msg.data[tp_dcs_offset]);
    *out_dcs = msg->msg.data[tp_dcs_offset];
    msg_out->dcs = msg->msg.data[tp_dcs_offset]; // We'll keep the DCS value
    switch (user_data_encoding) {
    case MM_SMS_ENCODING_GSM7:
      logger(MSG_PDU_DECODE, " - Encoding: GSM7\n");
      break;
    case MM_SMS_ENCODING_UCS2:
      logger(MSG_PDU_DECODE, " - Encoding: UCS2\n");
      break;
    case MM_SMS_ENCODING_8BIT:
      logger(MSG_PDU_DECODE, " - Encoding: 8bit\n");
      break;
    case MM_SMS_ENCODING_UNKNOWN:
      logger(MSG_PDU_DECODE, " - Encoding: unknown\n");
      break;
    default:
      logger(MSG_PDU_DECODE,
             "%s: Error getting the coding scheme, bailing out (dcs %.2x)!\n",
             msg_out->dcs);
      return -1;
    }

    /* Class */
    if (msg->msg.data[tp_dcs_offset] & SMS_DCS_CLASS_VALID)
      logger(MSG_PDU_DECODE, " - Message class is set: %u\n",
             msg->msg.data[tp_dcs_offset] & SMS_DCS_CLASS_MASK);
    else
      logger(MSG_PDU_DECODE, " - MESSAGE CLASS IS NOT SET\n");
  }

  logger(MSG_PDU_DECODE, " - User data length offset: %.2x, contents: %.2x\n",
         tp_user_data_len_offset, msg->msg.data[tp_user_data_len_offset]);
  if (tp_user_data_len_offset > 0) {
    uint8_t tp_user_data_size_elements;
    uint8_t tp_user_data_size_bytes;
    uint8_t tp_user_data_offset;
    uint8_t bit_offset;
    if (size < tp_user_data_len_offset + 1) {
      logger(MSG_ERROR, "%s: Packet too short reading user data length\n",
             __func__);
      return -ENOMEM;
    }
    tp_user_data_size_elements = msg->msg.data[tp_user_data_len_offset];
    logger(MSG_PDU_DECODE, "UDL: %u elements\n", tp_user_data_size_elements);

    if (user_data_encoding == MM_SMS_ENCODING_GSM7)
      tp_user_data_size_bytes = (7 * (tp_user_data_size_elements + 1)) / 8;
    else
      tp_user_data_size_bytes = tp_user_data_size_elements;

    logger(MSG_PDU_DECODE, "UDL: %u bytes\n", tp_user_data_size_bytes);

    tp_user_data_offset = tp_user_data_len_offset + 1;
    if (size < tp_user_data_offset + 1) {
      logger(MSG_ERROR, "%s: Packet too short when reading user data!\n",
             __func__);
      return -ENOMEM;
    }
    bit_offset = 0;
    if (has_udh) {
      logger(MSG_PDU_DECODE, "** Message contains user data header** \n");
      uint8_t udhl, end;

      udhl = msg->msg.data[tp_user_data_offset] + 1;
      end = tp_user_data_offset + udhl;
      logger(MSG_PDU_DECODE, "UDHL:: Size: %.2x \n",
             msg->msg.data[tp_user_data_offset]);
      logger(MSG_PDU_DECODE, "UDHL:: END: %.2x \n",
             msg->msg.data[tp_user_data_offset + udhl]);
      if (size < tp_user_data_offset + udhl) {
        logger(MSG_ERROR, "%s: UDH: Packet too small!\n", __func__);
        return -ENOMEM;
      }

      for (offset = tp_user_data_offset + 1; (offset + 1) < end;) {
        uint8_t ie_id, ie_len;
        ie_id = msg->msg.data[offset++];
        ie_len = msg->msg.data[offset++];
        logger(MSG_PDU_DECODE, "IE: %.2x size %.2x\n", ie_id, ie_len);
        switch (ie_id) {
        case 0x00:
          if (offset + 2 >= end)
            break;
          logger(MSG_PDU_DECODE,
                 "Concatenated message: Part %u of %u (seq: %u)\n",
                 msg->msg.data[offset + 2], msg->msg.data[offset + 1],
                 msg->msg.data[offset + 3]);
          /*
           * Ignore the IE if one of the following is true:
           *  - it claims to be part 0 of M
           *  - it claims to be part N of M, N > M
           */
          if (msg->msg.data[offset + 2] == 0 ||
              msg->msg.data[offset + 2] > msg->msg.data[offset + 1])
            break;
          break;
        case 0x08:
          if (offset + 3 >= end)
            break;
          /* Concatenated short message, 16-bit reference */
          if (msg->msg.data[offset + 3] == 0 ||
              msg->msg.data[offset + 3] > msg->msg.data[offset + 2])
            break;
          logger(MSG_PDU_DECODE,
                 "Concatenated (16b) message: Part %u of %u (seq: %u)\n",
                 (msg->msg.data[offset] << 8) | msg->msg.data[offset + 1],
                 msg->msg.data[offset + 2], msg->msg.data[offset + 3]);
          break;
        default:
          logger(MSG_PDU_DECODE, "Single part message \n");
          break;
        }

        offset += ie_len;
      }

      /*
       * Move past the user data headers to prevent it from being
       * decoded into garbage text.
       */
      logger(MSG_PDU_DECODE, "Current offset: %u, data %.2x\n", offset,
             msg->msg.data[offset]);
      logger(MSG_PDU_DECODE, "User data offset: %u | data size bytes: %u\n",
             tp_user_data_offset, tp_user_data_size_bytes);
      logger(MSG_PDU_DECODE, "UDHL: %u\n", udhl);
      tp_user_data_offset += udhl;
      tp_user_data_size_bytes -= udhl;
      logger(MSG_PDU_DECODE, "User data offset: %u | data size bytes: %u\n",
             tp_user_data_offset, tp_user_data_size_bytes);
      if (user_data_encoding == MM_SMS_ENCODING_GSM7) {
        logger(MSG_PDU_DECODE, "Encoding is GSM7\n");
        /*
         * Find the number of bits we need to add to the length of the
         * user data to get a multiple of 7 (the padding).
         */
        bit_offset = (7 - udhl % 7) % 7;
        logger(MSG_PDU_DECODE, "Bit offset is %.2x\n", bit_offset);
        tp_user_data_size_elements -= (udhl * 8 + bit_offset) / 7;
      } else {
        logger(MSG_PDU_DECODE, "Encoding !== GSM7\n");
        tp_user_data_size_elements -= udhl;
      }
    }

    switch (user_data_encoding) {
    case MM_SMS_ENCODING_GSM7: {
      char text[160] = {0};

      /* Otherwise if it's 7-bit or UCS2 we can decode it */
      logger(MSG_PDU_DECODE,
             "GSM7: Decoding SMS text with %u elements (%u bytes) \n",
             tp_user_data_size_elements, tp_user_data_size_bytes);
      gsm7_to_ascii(&msg->msg.data[tp_user_data_offset],
                    tp_user_data_size_elements, text, tp_user_data_size_bytes,
                    bit_offset);
      logger(MSG_PDU_DECODE, "Decoded string: %s\n", text);
      strncpy(msg_out->message, text, 160);
      msg_out->raw_message = false;
      break;
    }
    case MM_SMS_ENCODING_UCS2:
    case MM_SMS_ENCODING_8BIT:
    case MM_SMS_ENCODING_UNKNOWN:
    default: {
      logger(MSG_WARN,
             "%s: Not converting message contents, passing it raw (encoding: "
             "%.2x)\n",
             __func__, user_data_encoding);
      uint8_t raw_data[160];
      if (tp_user_data_size_bytes > 160) {
        logger(MSG_ERROR, "ERROR: Size exceeds what fits in a SMS\n");
      } else {
        memcpy(raw_data, msg->msg.data + tp_user_data_offset,
               tp_user_data_size_bytes);
        msg_out->raw_message = true;
      }
      break;
    }
    }
  }

  return 0;
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

void fill_sender_phone_number(uint8_t *number, bool alternate_num) {
  number[0] = 0x22;
  number[1] = 0x33;
  number[2] = 0x44;
  number[3] = 0x55;
  number[4] = 0x66;
  number[5] = 0x77;
  if (alternate_num) {
    number[5] = 0x87;
  }
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

  if (sms_runtime.queue.msg[message_id].is_cb)
    fill_sender_phone_number(this_sms->data.smsc.number, true);
  else
    fill_sender_phone_number(this_sms->data.smsc.number, false);

  this_sms->data.unknown = 0x04; // This is still unknown

  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   * 4 bits for each number, backwards  */

  /* PHONE NUMBER */
  this_sms->data.phone.phone_number_size = 0x0c;       // hardcoded
  this_sms->data.phone.is_international_number = 0x91; // yes

  if (sms_runtime.queue.msg[message_id].is_cb)
    fill_sender_phone_number(this_sms->data.phone.number, true);
  else
    fill_sender_phone_number(this_sms->data.phone.number, false);

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
 * Build and send SMS without touching the contents (for UCS2/8B encoded
 * messages) Gets message ID, builds the QMI messages and sends it Returns
 * numnber of bytes sent. Since oFono tries to read the message an arbitrary
 * number of times, or delete it or whatever, we need to keep them a little
 * longer on hold...
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
  if (sms_runtime.queue.msg[message_id].is_cb)
    fill_sender_phone_number(this_sms->data.smsc.number, true);
  else
    fill_sender_phone_number(this_sms->data.smsc.number, false);
  // ENCODING TEST
  this_sms->data.unknown = 0x04; // This is still unknown

  // We leave all this hardcoded, we will only worry about ourselves
  /* We need a hardcoded number so when a reply comes we can catch it,
   * otherwise we would be sending it off to the baseband!
   * 4 bits for each number, backwards  */
  /* PHONE NUMBER */
  this_sms->data.phone.phone_number_size = 0x0c;       // hardcoded
  this_sms->data.phone.is_international_number = 0x91; // yes

  if (sms_runtime.queue.msg[message_id].is_cb)
    fill_sender_phone_number(this_sms->data.phone.number, true);
  else
    fill_sender_phone_number(this_sms->data.phone.number, false);

  this_sms->data.tp_pid = 0x00;
  this_sms->data.tp_dcs = 0x00;
  if (sms_runtime.queue.msg[message_id].tp_dcs != 0x00) {
    logger(MSG_INFO, "%s: UCS/8Bit %.2x\n", __func__,
           sms_runtime.queue.msg[message_id].tp_dcs);
    this_sms->data.tp_dcs = sms_runtime.queue.msg[message_id].tp_dcs;
    if (sms_runtime.queue.msg[message_id].len < MAX_MESSAGE_SIZE &&
        sms_runtime.queue.msg[message_id].len % 2 != 0) {
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
  this_sms->data.contents.content_sz = sms_runtime.queue.msg[message_id].len;

  if (sms_runtime.queue.msg[message_id].tp_dcs == 0x00) {
    logger(MSG_WARN, "*** RAWSMS: Content sz: %i -> to8 -> %i",
           sms_runtime.queue.msg[message_id].len,
           (sms_runtime.queue.msg[message_id].len * 8) / 7);
    this_sms->data.contents.content_sz =
        (sms_runtime.queue.msg[message_id].len * 8) / 7;
  }

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
    pulse_ring_in();
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
    if (!sms_runtime.queue.msg[message_id].is_raw) {
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
    logger(MSG_DEBUG, "%s: WMS_EVENT_REPORT for message %i. ID %.4x\n",
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
    offset = get_tlv_offset_by_id(bytes, len, 0x01);
    if (offset > 0) {
      struct sms_storage_type *storage;
      storage = (struct sms_storage_type *)(bytes + offset);
      logger(MSG_WARN, "%s: Message ID: (offset) 0x%.2x\n", __func__,
             storage->message_id);
      sms_runtime.current_message_id = storage->message_id;
      sms_runtime.queue.msg[sms_runtime.current_message_id].state = 2;
      handle_message_state(fd, sms_runtime.current_message_id);
      clock_gettime(
          CLOCK_MONOTONIC,
          &sms_runtime.queue.msg[sms_runtime.current_message_id].timestamp);

    } else {
      logger(MSG_ERROR, "%s: Can't find offset for raw_message!\n", __func__);
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
  case WMS_LIST_ALL_MESSAGES:
    logger(MSG_INFO, "Host requests to list ALL messages");
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
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].is_raw = 0;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].is_cb = 0;

  } else {
    logger(MSG_ERROR, "%s: Size of message is 0\n", __func__);
  }
}

void add_raw_sms_to_queue(uint8_t *message, size_t len, uint8_t tp_dcs,
                          bool is_cb) {
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
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].len = len;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].tp_dcs = tp_dcs;
    sms_runtime.queue.msg[sms_runtime.queue.queue_pos].is_raw = 1;
    if (is_cb) {
      sms_runtime.queue.msg[sms_runtime.queue.queue_pos].is_cb = 1;
    }
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
      uint8_t tp_user_data_size_elements = pkt->contents.content_sz;
      uint8_t tp_user_data_size_bytes =
          (7 * (tp_user_data_size_elements + 1)) / 8;
      ret = gsm7_to_ascii(pkt->contents.contents, tp_user_data_size_elements,
                          (char *)output, tp_user_data_size_bytes, 0);
      if (ret < 0) {
        logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
               __LINE__);
      }
    } else if (pkt->pdu_type == 0x01) {
      uint8_t tp_user_data_size_elements = nodate_pkt->contents.content_sz;
      uint8_t tp_user_data_size_bytes =
          (7 * (tp_user_data_size_elements + 1)) / 8;
      ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                          tp_user_data_size_elements, (char *)output,
                          tp_user_data_size_bytes, 0);
      if (ret < 0) {
        logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
               __LINE__);
      }
    } else {
      set_log_level(MSG_DEBUG);

      logger(MSG_ERROR,
             "%s: Don't know how to handle this. Please contact biktorgj and "
             "get him the following dump:\n",
             __func__);
      dump_pkt_raw(bytes, len);
      logger(MSG_ERROR,
             "%s: Don't know how to handle this. Please contact biktorgj and "
             "get him the following dump:\n",
             __func__);
    }
    set_log_level(MSG_INFO);
    send_outgoing_msg_ack(pkt->qmipkt.transaction_id, usbfd, 0x0000);
    parse_command(output);
  }
  pkt = NULL;
  nodate_pkt = NULL;
  free(output);
  return 0;
}

/* Sniff on an sms */
uint8_t log_message_contents(uint8_t source, void *bytes, size_t len) {
  uint8_t *output;
  uint8_t ret;
  uint8_t dcs;
  struct message_data *msg_out =
      malloc(sizeof(struct message_data)); // 2022-12-20
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
                            (char *)output, pkt->contents.content_sz, 0);
        if (ret < 0) {
          logger(MSG_ERROR, "%s: %i: Failed to convert to ASCII\n", __func__,
                 __LINE__);
        }
      } else if (pkt->pdu_type == 0x01) {
        ret = gsm7_to_ascii(nodate_pkt->contents.contents,
                            strlen((char *)nodate_pkt->contents.contents),
                            (char *)output, nodate_pkt->contents.content_sz, 0);
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
    if (pdu_decode(bytes, len, msg_out, &dcs) < 0) {
        logger(MSG_ERROR, "%s: [dsp] Error decoding the PDU! is this an outgoing message from the host?\n", __func__);
    }
  }
  free(output);
  free(msg_out);
  return 0;
}
int check_wms_message(uint8_t source, void *bytes, size_t len, int adspfd,
                      int usbfd) {
  uint8_t our_phone[] = {0x91, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t cb_phone[] = {0x91, 0x22, 0x33, 0x44, 0x55, 0x66, 0x78};
  int needs_rerouting = 0;
  struct outgoing_sms_packet *pkt;
  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    // is it for us?
    if (memcmp(pkt->target.phone_number, our_phone,
               sizeof(pkt->target.phone_number)) == 0 ||
        memcmp(pkt->target.phone_number, cb_phone,
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

/*
READ MESSAGE REQ
0x01
0x18 0x00
0x00
0x05
0x01

0x00
0x0a 0x00
0x22 0x00
0x0c 0x00

0x10
0x01 0x00
0x01

0x01
0x05 0x00
0x01 0x01 0x00 0x00 0x00

// Recreated:

0x01,
0x18, 0x00,
0x00,
0x05,
0x01,

0x00,
0x00, 0x00,
0x22, 0x00,
0x0c, 0x00
[WHYYYYYYY]
0x01,
0x01, 0x00,
0x00,
[ This is hardcoded to 0x10, 0x01, 0x00, 0x01 ^^]
0x01,
0x05, 0x00,
0x01, 0x01, 0x00, 0x00, 0x00,

*/

int parse_and_forward_adsp_message(void *bytes, size_t len) {
  struct message_data *msg_out =
      malloc(sizeof(struct message_data)); // 2022-12-20
  uint8_t *message = malloc(160 * sizeof(uint8_t));
  uint8_t dcs = 0;
  int sztmp;
  struct raw_sms *msg = (struct raw_sms *)bytes;

  if (len < sizeof(struct raw_sms)) {
    logger(MSG_ERROR, "%s: Response is too small!\n", __func__);
    free(msg_out);
    free(message);
    return -EINVAL;
  }

  if (msg->qmipkt.msgid != WMS_READ_MESSAGE) {
    logger(MSG_ERROR, "%s: Invalid message ID (0x%.4x)\n", __func__,
           msg->qmipkt.msgid);
    free(msg_out);
    free(message);
    return -EINVAL;
  }

  if (pdu_decode(bytes, len, msg_out, &dcs) < 0) {
    logger(MSG_ERROR, "%s: Failed to decode the PDU!\n", __func__);
    free(msg_out);
    free(message);
    return -EINVAL;
  } else {
    logger(MSG_INFO, "Regenerate stuck message and send it!\n");
    sztmp = snprintf((char *)message, 160, "Stuck message from %s:\n--------\n",
                     msg_out->phone_number);
    add_sms_to_queue((uint8_t *)message, sztmp);
    memset(message, 0, 160);
    if (dcs != 0) {
      logger(MSG_WARN, "%s: Sending message as a raw message, hope for the best\n", __func__);
      add_raw_sms_to_queue(message, sztmp, dcs, true);
    } else {
      logger(MSG_WARN, "%s: Sending the message as a pre-parsed GSM7 message\n", __func__);
      sztmp = snprintf((char *)message, 160, "%s", msg_out->message);
      add_sms_to_queue((uint8_t *)message, sztmp);

    }
  }

  free(msg_out);
  free(message);
  return 0;
}

int request_message_delete(int adspfd, uint8_t instance_id,
                           uint16_t transaction_id, uint32_t message_id) {
  struct timeval tv;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  int ret;
  struct sms_delete_request *request =
      calloc(1, sizeof(struct sms_delete_request));

  /* Message request */
  // QMUX
  request->qmuxpkt.version = 0x01;
  request->qmuxpkt.packet_length =
      0x1b; // Request size is hardcoded as it's always the same size
  request->qmuxpkt.control = 0x00;
  request->qmuxpkt.service = 0x05; // WMS Service
  request->qmuxpkt.instance_id =
      instance_id; // Current connected instance from the host (we're hijacking
                   // the session here)

  // QMI
  request->qmipkt.ctlid = 0x00;
  request->qmipkt.transaction_id = transaction_id;
  request->qmipkt.msgid = WMS_DELETE;
  request->qmipkt.length = 0x0f;

  // SMS over IMS?
  request->sms_mode.id = 0x12;
  request->sms_mode.len = 0x01;
  request->sms_mode.type = 0x01; // GW_PP

  // Index
  request->index.id = 0x10;
  request->index.len = 0x04;
  request->index.message_id = message_id;

  // Storage type
  request->sms_type.id = 0x01;
  request->sms_type.len = 0x01;
  request->sms_type.type = 0x01;

  logger(MSG_INFO, "%s: Requesting deletion of %u\n", __func__, message_id);
  dump_pkt_raw((uint8_t *)request, sizeof(struct sms_delete_request));
  ret = write(adspfd, request, sizeof(struct sms_delete_request));
  if (!ret) {
    logger(MSG_ERROR, "%s: Error sending request (ret %i)\n", __func__, ret);
    return -EINVAL;
  }
  logger(MSG_INFO, "Request sent (%i bytes)\n", ret);
  tv.tv_sec = 1;
  tv.tv_usec = 500000;
  FD_SET(adspfd, &readfds);
  select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(adspfd, &readfds)) {
    ret = read(adspfd, &buf, MAX_PACKET_SIZE);
    if (ret > 0) {
      /* We got an answer*/
      logger(MSG_INFO, "Deletion ACK received:\n");
      dump_pkt_raw(buf, ret);
    } else {
      logger(MSG_ERROR, "%s: Didn't get message %u (empty read)\n", __func__,
             message_id);
      return -EINVAL;
    }
  } else {
    logger(
        MSG_ERROR,
        "**** %s: Couldn't delete message %u (req failed, no answer from the "
        "baseband)\n",
        __func__, message_id);
    return -EINVAL;
  }
  free(request);
  return 0;
}

/*
 * Sends a request to the ADSP to retrieve a message
 * If it succeeds, it will send the response to
 *  parse_adsp_message()
 * Otherwise it will return -EINVAL
 */
int request_read_adsp_message(int adspfd, uint8_t instance_id,
                              uint16_t transaction_id, uint32_t message_id) {
  struct timeval tv;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  int ret;
  struct sms_get_message_by_id *request =
      calloc(1, sizeof(struct sms_get_message_by_id));

  /* Message request */
  // QMUX
  request->qmuxpkt.version = 0x01;
  request->qmuxpkt.packet_length =
      0x18; // Request size is hardcoded as it's always the same size
  request->qmuxpkt.control = 0x00;
  request->qmuxpkt.service = 0x05; // WMS Service
  request->qmuxpkt.instance_id =
      instance_id; // Current connected instance from the host (we're hijacking
                   // the session here)

  // QMI
  request->qmipkt.ctlid = 0x00;
  request->qmipkt.transaction_id = transaction_id;
  request->qmipkt.msgid = WMS_READ_MESSAGE;
  request->qmipkt.length = 0x0c;

  // Mode
  request->mode.id = 0x10;
  request->mode.len = 0x01;
  request->mode.type = 0x01; // GW_PP

  // Index
  request->index.id = 0x01;
  request->index.len = 0x05;
  request->index.type = 0x01; // NV
  request->index.message_id = message_id;

  logger(MSG_INFO, "%s: Requesting message id %u\n", __func__, message_id);
  dump_pkt_raw((uint8_t *)request, sizeof(struct sms_get_message_by_id));
  ret = write(adspfd, request, sizeof(struct sms_get_message_by_id));
  if (!ret) {
    logger(MSG_ERROR, "%s: Error sending request (ret %i)\n", __func__, ret);
    return -EINVAL;
  }
  logger(MSG_INFO, "Request sent (%i bytes)\n", ret);
  tv.tv_sec = 1;
  tv.tv_usec = 500000;
  FD_SET(adspfd, &readfds);
  select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(adspfd, &readfds)) {
    ret = read(adspfd, &buf, MAX_PACKET_SIZE);
    if (ret > 0) {
      /* We got an answer*/
      free(request);
      return parse_and_forward_adsp_message(buf, ret);
    } else {
      logger(MSG_ERROR, "%s: Didn't get message %u (empty read)\n", __func__,
             message_id);
      return -EINVAL;
    }
  } else {
    logger(MSG_ERROR,
           "%s: Couldn't get message %u (req failed, no answer from the "
           "baseband)\n",
           __func__, message_id);
    return -EINVAL;
  }
  free(request);
  return 0;
}

/*
 * If _List All Messages_ returned data, we will iterate through it
 * and send read requests for each message. We will then parse them
 * and request to delete them
 */
int retrieve_and_delete(int adspfd, int usbfd) {
  struct sms_list_all_resp *pkt;
  uint16_t transaction_id = 1;
  struct sms_get_message_by_id *thismsg =
      calloc(1, sizeof(struct sms_get_message_by_id));
  logger(MSG_INFO, " *** %s start *** \n", __func__);
  logger(MSG_WARN,
         "This function is in testing stage. Enabling debug mode now\n");
  set_log_level(MSG_DEBUG);
  if (sms_runtime.stuck_message_data != NULL) {
    logger(MSG_WARN,
           "There are stale messages stored in the Modem's baseband\n");
    pkt = (struct sms_list_all_resp *)sms_runtime.stuck_message_data;
    for (int i = 0; i < pkt->info.number_of_messages_listed; i++) {
      struct sms_list_all_message_data *this_message = pkt->data + i;
      logger(MSG_INFO, "We found data for message id %u of type %u\n",
             this_message->message_id, this_message->message_type);
      if (request_read_adsp_message(adspfd, pkt->qmuxpkt.instance_id,
                                    transaction_id,
                                    this_message->message_id) == 0) {
        logger(MSG_INFO, "%s: We got your message back!\n", __func__);
        transaction_id++;
        if (request_message_delete(adspfd, pkt->qmuxpkt.instance_id,
                                   transaction_id,
                                   this_message->message_id) == 0) {
          logger(MSG_INFO, "%s: All done for message %u\n", __func__,
                 this_message->message_id);
        }
      } else {
        logger(MSG_ERROR, "%s: We couldn't get the message from the baseband\n",
               __func__);
        transaction_id += 2;
      }
      this_message = NULL;
    }
  } else {
    logger(MSG_INFO, "******  Nothing to do here\n");
  }

  sms_runtime.stuck_message_data_pending = false;
  logger(MSG_INFO, "%s finished, disabling debug mode\n", __func__);
  set_log_level(MSG_INFO);
  free(thismsg);
  return 0;
}
/*
raw_pkt[] = {
  QMUX header
  0x01,
  0x29, 0x00,
  0x80,
  0x05,
  0x01,

  QMI header
  0x02,
  0x09, 0x00,
  0x31, 0x00, <-- List all messages request/response
  0x1d, 0x00,

  Our actual message

  0x02, <-- QMI result (OK)
  0x04, 0x00,
  0x00, 0x00, 0x00, 0x00,

  0x01, <-- TLV ID 0x01: Data about all the available messages in memory
  0x13, 0x00, <-- Size
  0x03, 0x00, 0x00, 0x00, <- u32 Number of messages in memory
  |--- message id -----| |msg type|
  0x00, 0x00, 0x00, 0x00, 0x00, <-- Stuck message with ID 0
  0x01, 0x00, 0x00, 0x00, 0x00, <-- Stuck message with ID 1
  0x02, 0x00, 0x00, 0x00, 0x00, <-- Stuck message with ID 2

  After List all messages, the ADSP will directly send WMS_READ_MESSAGE
  indications with the contents of all the messages it has in sequential
  order, and with consecutive transaction_ids. We can use that to track
  if the following requests come as part of the message and treat it all
  as one single operation
*/
int check_wms_list_all_messages(uint8_t source, void *bytes, size_t len,
                                int adspfd, int usbfd) {
  int ret;
  int needs_bypass = 0;
  uint32_t i = 0;
  struct sms_list_all_resp *pkt;
  if (source == FROM_DSP) {
    struct sms_list_all_resp_empty *empty_answer =
        calloc(1, sizeof(struct sms_list_all_resp_empty));
    if (len >= sizeof(struct sms_list_all_resp)) {
      pkt = (struct sms_list_all_resp *)bytes;
      if (pkt->qmipkt.msgid == WMS_LIST_ALL_MESSAGES) {
        logger(MSG_INFO, "%s: Host requests all messages\n", __func__);
        logger(MSG_INFO, "Total messages in memory: %u\n",
               pkt->info.number_of_messages_listed);
        for (i = 0; i < pkt->info.number_of_messages_listed; i++) {
          struct sms_list_all_message_data *this_message = pkt->data + i;
          logger(MSG_INFO, "We found data for message id %u of type %u\n",
                 this_message->message_id, this_message->message_type);
          this_message = NULL;
        }
        if (i > 0) {
          logger(MSG_INFO, "Saving this message to trigger deletion later\n");
          if (sms_runtime.stuck_message_data != NULL) {
            free(sms_runtime.stuck_message_data);
          }
          sms_runtime.stuck_message_data = malloc(len);
          memcpy(sms_runtime.stuck_message_data, bytes, len);

          logger(
              MSG_WARN,
              "The DSP wants to give us some messages, we're going to lie\n");
          sms_runtime.stuck_message_data_pending = true;
          needs_bypass = 1;
          empty_answer->qmuxpkt = pkt->qmuxpkt;
          empty_answer->qmipkt = pkt->qmipkt;
          empty_answer->qmuxpkt.packet_length = 0x1a;
          empty_answer->qmipkt.length = 0x0e;
          empty_answer->indication.result_code_type = 0x02;
          empty_answer->indication.generic_result_size = 0x04;
          empty_answer->info.type = 0x01;
          empty_answer->info.len = 0x04;

          ret = write(usbfd, empty_answer,
                      sizeof(struct sms_list_all_resp_empty));
          if (ret < 0) {
            logger(MSG_ERROR, "%s: Failed to write the simulated reply\n",
                   __func__);
          }
        }
      }
    }
          free(empty_answer);
  }
  return needs_bypass;
}

/* Intercept and ACK a message */
uint8_t intercept_cb_message(void *bytes, size_t len) {
  uint8_t *output;
  struct cell_broadcast_message_prototype *pkt;

  output = calloc(MAX_CB_MESSAGE_SIZE, sizeof(uint8_t));

  if (len >= sizeof(struct cell_broadcast_message_prototype) -
                 (MAX_CB_MESSAGE_SIZE + 2)) {
    logger(MSG_INFO, "%s: Message size is big enough\n", __func__);

    pkt = (struct cell_broadcast_message_prototype *)bytes;
    if (pkt->header.id == TLV_TRANSFER_MT_MESSAGE && pkt->message.id == 0x07) {
      logger(MSG_INFO, "%s: TLV ID matches, trying to decode\n", __func__);
      logger(MSG_WARN,
             "CB Message dump:\n--\n"
             " ID: 0x%.2x\n "
             " LEN: 0x%.4x\n "
             " - Serial %i\n"
             " - Message ID %i\n"
             " - Encoding: 0x%.2x\n"
             " - Page %.2x\n",
             pkt->header.id, pkt->header.len, pkt->message.pdu.serial_number,
             pkt->message.pdu.message_id, pkt->message.pdu.encoding,
             pkt->message.pdu.page_param);

      set_log_level(MSG_DEBUG);
      logger(MSG_DEBUG, "%s: CB MESSAGE DUMP\n", __func__);
      dump_pkt_raw(bytes, len);
      logger(MSG_DEBUG, "%s: CB MESSAGE DUMP END\n", __func__);
      set_log_level(MSG_INFO);
      if ((pkt->message.pdu.page_param >> 4) == 0x01)
        add_message_to_queue(
            (uint8_t
                 *)"WARNING: Incoming Cell Broadcast Message from the network",
            strlen(
                "WARNING: Incoming Cell Broadcast Message from the network"));
      /*
       * Just in case someone looks at this code looking for hints on something
       * else: If the upper bits of the encoding u8 are 01XX, it means that the
       * encoding field has actually some specific encoding selected, wether it
       * is GSM-7, UCS-2 or a TE-Specific encoding SMS and CB encoding u8's
       * don't follow the exact same scheme, but they share bits 2 and 3 to set
       * the encoding. Since we're manipulating the original message here, I
       * just clear out part of the bits so ModemManager actually thinks it's a
       * SMS but I keep the original encoding bits and shove it back as a (raw)
       * SMS into my code for processing via the proxy. With this I don't have
       * to actually decode and reencode the message, or play around with
       * multipart SMS or whatever, I delegate that to the host manager and be
       * done with it
       */

      if (!(pkt->message.pdu.encoding & (1 << 7)) &&
          (pkt->message.pdu.encoding & (1 << 6))) { // 01xx
        logger(MSG_WARN, "%s:Message encoding is UCS-2 or TE-Specific\n",
               __func__);
      } else {
        logger(MSG_WARN,
               "%s:Either there's no encoding selected or it is GSM-7\n",
               __func__);
      }
      int sz = pkt->message.len - 6;
      if (sz > MAX_MESSAGE_SIZE)
        memcpy(output, pkt->message.pdu.contents, MAX_MESSAGE_SIZE);
      else
        memcpy(output, pkt->message.pdu.contents, sz);

      int sms_dcs = pkt->message.pdu.encoding;
      /* We need to make sure certain bits of the data coding scheme are set to
       * 0 to avoid confusing ModemManager...
       *   https://www.etsi.org/deliver/etsi_ts/123000_123099/123038/10.00.00_60/ts_123038v100000p.pdf
       * We leave alone bit #5 (Compression), and #3 and #2, which define the
       * encoding type (GSM7 || 8bit || UCS2) We clear the rest
       */
      sms_dcs &= ~(1UL << 7); // We already know encoding is set to 01xx (CB,
                              // section 5, page 12), SMS needs
      sms_dcs &= ~(1UL << 6); // this to be to 00xx (SMS, Section 4, page 8) to
                              // match data coding scheme
      //            sms_dcs &= ~(1UL << 5); // We leave this bit as is,
      //            *compression*
      sms_dcs &=
          ~(1UL << 4); // We clear the message class bit, we don't need this
      //            sms_dcs &= ~(1UL << 3); // We keep these, as they set the
      //            encoding type sms_dcs &= ~(1UL << 2); // GSM7 / UCS2 / TE
      //            Specific
      sms_dcs &= ~(1UL << 1); // We clear these, as they are related to bit 4
      sms_dcs &= ~(1UL << 0); // we just cleared before.
      logger(MSG_WARN, "%s: Setting DCS from %.2x to %.2x\n", __func__,
             pkt->message.pdu.encoding, sms_dcs);
      add_raw_sms_to_queue(output, sz, sms_dcs, true);
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


void send_hello_world() {
  char message[160];
  snprintf(message, 160,
           "Hi!\nWelcome to your (nearly) free modem\nSend \"help\" in this "
           "chat to get a list of commands you can run");
  clear_ifrst_boot_flag();
  add_message_to_queue((uint8_t *)message, strlen(message));
}