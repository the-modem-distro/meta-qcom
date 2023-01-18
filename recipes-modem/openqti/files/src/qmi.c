// SPDX-License-Identifier: MIT
#include "../inc/qmi.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"

#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

struct {
  uint8_t is_initialized;
  int fd;
  struct qmi_service_bindings services[QMI_SERVICES_LAST];
} internal_qmi_client;

/*
 *
 * Utilities for QMI messages
 *
 *
 *
 *
 *
 */

/* Get service from QMUX header */
uint8_t get_qmux_service_id(void *bytes, size_t len) {
  struct qmux_packet *pkt = (struct qmux_packet *)bytes;
  return pkt->service;
}

/* Get instance from QMUX header */
uint8_t get_qmux_instance_id(void *bytes, size_t len) {
  struct qmux_packet *pkt = (struct qmux_packet *)bytes;
  return pkt->instance_id;
}
/* Get Message ID from a QMI control message*/
uint16_t get_control_message_id(void *bytes, size_t len) {
  struct ctl_qmi_packet *pkt =
      (struct ctl_qmi_packet *)(bytes + sizeof(struct qmux_packet));
  logger(MSG_DEBUG,
         "Control message: CTL %.2x | TID: %.2x | MSG %.4x | LEN %.4x\n",
         pkt->ctlid, pkt->transaction_id, pkt->msgid, pkt->length);
  return pkt->msgid;
}

/* Get Message ID for a QMI message (from a service) */
uint16_t get_qmi_message_id(void *bytes, size_t len) {
  struct qmi_packet *pkt =
      (struct qmi_packet *)(bytes + sizeof(struct qmux_packet));
  return pkt->msgid;
}

/* Get Message ID for a QMI message (from a service) */
uint16_t get_qmi_transaction_id(void *bytes, size_t len) {
  struct qmi_packet *pkt =
      (struct qmi_packet *)(bytes + sizeof(struct qmux_packet));
  return pkt->transaction_id;
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
 * If invalid, returns 0, which would always be an invalid offset
 */
uint16_t get_tlv_offset_by_id(uint8_t *bytes, size_t len, uint8_t tlvid) {
  uint16_t cur_byte;
  uint8_t *arr = (uint8_t *)bytes;
  struct empty_tlv *this_tlv;
  if (len < sizeof(struct encapsulated_qmi_packet) + 4) {
    logger(MSG_ERROR, "%s: Packet is too small \n", __func__);
    return 0;
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
  return 0;
}

/*
 * Gets error string from a result_code inside a QMI indication
 * TLV (0x02)
 */
const char *get_qmi_error_string(uint16_t result_code) {
  for (int j = 0; j < (sizeof(qmi_error_codes) / sizeof(qmi_error_codes[0]));
       j++) {
    if (qmi_error_codes[j].code == result_code) {
      return qmi_error_codes[j].error_name;
    }
  }
  return "Unknown error";
}

/*
 * Checks if a QMI indication is present in a QMI message (0x02)
 * If there is, returns if the operation succeeded or not, and
 * prints the error response to the log, if any.
 */
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
    if (this_tlv->result_code_type == 0x02 &&
        this_tlv->generic_result_size == 0x04) {
      result = this_tlv->result;
      if (this_tlv->result == QMI_RESULT_FAILURE) {
        logger(MSG_ERROR, "** QMI OP Failed: Code 0x%.4x (%s)\n",
               this_tlv->response, get_qmi_error_string(this_tlv->response));
      } else {
        logger(MSG_INFO, "** QMI OP Succeeded: Code 0x%.4x (%s)\n",
               this_tlv->response, get_qmi_error_string(this_tlv->response));
      }
      arr = NULL;
      this_tlv = NULL;
      return result;
    }
    cur_byte += le16toh(this_tlv->generic_result_size) + sizeof(uint8_t) +
                sizeof(uint16_t);
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

int build_qmux_header(void *output, size_t output_len, uint8_t control,
                      uint8_t service, uint8_t instance) {
  if (output_len < sizeof(struct qmux_packet)) {
    logger(MSG_ERROR, "%s: Can't build QMUX header, buffer is too small\n",
           __func__);
    return -ENOMEM;
  }
  struct qmux_packet *pkt = (struct qmux_packet *)output;
  pkt->version = 0x01;
  pkt->packet_length =
      output_len -
      sizeof(uint8_t); // QMUX's stated length is entire size - version field
  pkt->control = control;
  pkt->service = service;
  pkt->instance_id = instance;
  pkt = NULL;
  return 0;
}

int build_qmi_header(void *output, size_t output_len, uint8_t ctlid,
                     uint16_t transaction_id, uint16_t message_id) {
  if (output_len < (sizeof(struct qmux_packet) + sizeof(struct qmi_packet))) {
    logger(MSG_ERROR, "%s: Can't build QMI header, buffer is too small\n",
           __func__);
    return -ENOMEM;
  }
  struct qmi_packet *pkt =
      (struct qmi_packet *)(output + sizeof(struct qmux_packet));
  pkt->ctlid = ctlid;
  pkt->transaction_id = transaction_id;
  pkt->msgid = message_id;
  pkt->length =
      output_len - sizeof(struct qmux_packet) - sizeof(struct qmi_packet);
  pkt = NULL;
  return 0;
}
/* INTERNAL QMI CLIENT */
size_t read_from_qmi_port(void *buff, size_t max_sz) {
  size_t bytes_read = 0;
  fd_set readfds;
  struct timeval tv;

  memset(buff, 0, max_sz);
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  FD_SET(internal_qmi_client.fd, &readfds);
  select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(internal_qmi_client.fd, &readfds)) {
    logger(MSG_INFO, "%s: Is set!\n", __func__);
    bytes_read = read(internal_qmi_client.fd, buff, max_sz);
  }
  set_log_level(0);
  dump_pkt_raw(buff, bytes_read);
  set_log_level(1);
  logger(MSG_INFO, "%s: Bytes read: %u\n", __func__, bytes_read);
  return bytes_read;
}

size_t write_to_qmi_port(void *buff, size_t bufsz) {
  size_t bytes_written = 0;
  set_log_level(0);
  dump_pkt_raw(buff, bufsz);
  set_log_level(1);
  bytes_written = write(internal_qmi_client.fd, buff, bufsz);
  if (bytes_written != bufsz) {
    logger(MSG_WARN,
           "%s: Heey we wrote %u bytes from %u that we were supposed to...\n",
           __func__, bytes_written, bufsz);
  }
  return bytes_written;
}

int allocate_qmi_client(uint8_t service) {
  uint8_t *buff = malloc(1024 * sizeof(uint8_t));
  struct client_alloc_request *request = NULL;
  size_t bytes = 0;
  int ret = 0;
  if (!internal_qmi_client.is_initialized) {
    internal_qmi_client.fd = open(INT_SMD_CNTL, O_RDWR);
    if (internal_qmi_client.fd < 0) {
      logger(MSG_ERROR, "Error opening %s!\n", INT_SMD_CNTL);
      free(buff);
      return -EINVAL;
    } else {
      logger(MSG_INFO, "%s: Opened internal SMD port\n", __func__);
      internal_qmi_client.is_initialized = 1;
    }
  }

  request = (struct client_alloc_request *)buff;
  request->qmux.version = 0x01;
  request->qmux.packet_length = 0x0f;
  request->qmux.service = 0x00;
  request->qmux.instance_id = 0x00;

  request->qmi.id = 0x06;
  request->qmi.msgid = 0x0022;
  request->qmi.size = 0x04;

  request->service.id = 0x01;
  request->service.len = 0x01;
  request->service.service_id = service; // 0x02 -> dms
  if (write_to_qmi_port((void *)request, sizeof(struct client_alloc_request)) <
      sizeof(struct client_alloc_request)) {
    logger(MSG_ERROR, "%s: Allocation failed\n", __func__);
    free(buff);
    return -ENOMEM;
  }
  memset(buff, 0, 1024);
  bytes = read_from_qmi_port(buff, 1024);
  if (bytes >= sizeof(struct client_alloc_response)) {
    logger(MSG_INFO, "We may have a valid response, let's check it\n");
    struct client_alloc_response *response =
        (struct client_alloc_response *)buff;
    if (response->result.response == 0x00 && response->result.result == 0x00) {
      logger(MSG_INFO, "%s: Yesss we allocated or client, instance ID: %.2x\n",
             __func__, response->instance.instance_id);
      internal_qmi_client.services[service].service =
          response->instance.service_id;
      internal_qmi_client.services[service].instance =
          response->instance.instance_id;
      internal_qmi_client.services[service].curr_transaction_id =
          response->qmi.transaction_id;
      internal_qmi_client.services[service].is_initialized = 1;
    }
  } else {
    logger(MSG_ERROR, "%s: Invalid allocation response: %u bytes\n", __func__,
           bytes);
  }
  free(buff);
  return ret;
}

void *init_internal_qmi_client() {
  int ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  struct timeval tv;
  uint8_t sample_demo = 0;
  size_t demo_pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *demo_pkt = malloc(demo_pkt_len);
  logger(MSG_WARN, "%s Trying to register to WDS\n", __func__);
  if (allocate_qmi_client(QMI_SERVICE_WDS) < 0) {
    logger(MSG_ERROR, "%s: Failed to allocate client: QMI_SERVICE_WDS\n",
           __func__);
  } else {
    logger(MSG_INFO, "%s: Connected to QMI_SERVICE_WDS\n", __func__);
  }

  logger(MSG_WARN, "%s Trying to register to DMS\n", __func__);
  if (allocate_qmi_client(QMI_SERVICE_DMS) < 0) {
    logger(MSG_ERROR, "%s: Failed to allocate client: QMI_SERVICE_DMS\n",
           __func__);
  } else {
    logger(MSG_INFO, "%s: Connected to QMI_SERVICE_DMS\n", __func__);
  }

  while (1) {
    logger(MSG_INFO, "%s: loop\n", __func__);
    FD_SET(internal_qmi_client.fd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(internal_qmi_client.fd, &readfds)) {
      ret = read(internal_qmi_client.fd, &buf, MAX_PACKET_SIZE);
      logger(MSG_INFO, "%s: New QMI Message of %i bytes\n", __func__, ret);
      pretty_print_qmi_pkt("Baseband --> Host", buf, ret);
    }
    memset(demo_pkt, 0, demo_pkt_len);
    if (build_qmux_header(
            demo_pkt, demo_pkt_len, 0x00, QMI_SERVICE_DMS,
            internal_qmi_client.services[QMI_SERVICE_DMS].instance) < 0) {
      logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    }
    switch (sample_demo) {
    case 0:
      if (build_qmi_header(demo_pkt, demo_pkt_len, 0x00, 0,
                           DMS_GET_REVISION) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      }
      break;
    case 1:
      if (build_qmi_header(demo_pkt, demo_pkt_len, 0x00, 1,
                           DMS_GET_SERIAL_NUM) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      }
      break;
    case 2:
      if (build_qmi_header(demo_pkt, demo_pkt_len, 0x00, 2,
                           DMS_GET_MODEL) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      }
      break;
    case 3:
      if (build_qmi_header(demo_pkt, demo_pkt_len, 0x00, 3,
                           DMS_GET_TIME) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      }
      break;

    default:
      break;
    }
    if (sample_demo < 4) {
      logger(MSG_INFO, "Writing test %i\n", sample_demo);
      write_to_qmi_port(demo_pkt, demo_pkt_len);
    }
    sample_demo++;
    if (sample_demo > 3) {
      sample_demo = 0;
      logger(MSG_INFO, "Restarting demo in 5s\n");
      sleep(5);
    }
  }

  return NULL;
}