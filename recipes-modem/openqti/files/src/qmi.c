// SPDX-License-Identifier: MIT
#include "../inc/qmi.h"
#include "../inc/call.h"
#include "../inc/cell.h"
#include "../inc/devices.h"
#include "../inc/dms.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"
#include "../inc/wds.h"
#include "../inc/voice.h"
#include "../inc/nas.h"
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
  uint8_t has_pending_message;
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

/* Set current instance in QMUX header and transaction id in qmi header*/
void prepare_internal_pkt(void *bytes, size_t len) {
  struct qmux_packet *pkt = (struct qmux_packet *)bytes;
  pkt->instance_id = internal_qmi_client.services[pkt->service].instance;
  struct qmi_packet *qmi =
      (struct qmi_packet *)(bytes + sizeof(struct qmux_packet));
  qmi->transaction_id =
      internal_qmi_client.services[pkt->service].transaction_id;

  pkt = NULL;
  qmi = NULL;
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

/* Get Message trype for a QMI message (from a service) */
uint16_t get_qmi_message_type(void *bytes, size_t len) {
  struct qmi_packet *pkt =
      (struct qmi_packet *)(bytes + sizeof(struct qmux_packet));
  return pkt->ctlid;
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

/*
 * Makes a QMUX header
 */
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

/*
 * Makes a QMI header for a service (not a control QMI header)
 */
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

int build_u8_tlv(void *output, size_t output_len, size_t offset, uint8_t id,
                 uint8_t data) {
  if (output_len < offset + sizeof(struct qmi_generic_uint8_t_tlv)) {
    logger(MSG_ERROR, "%s: Can't build U8 TLV, buffer is too small\n",
           __func__);
    return -ENOMEM;
  }
  struct qmi_generic_uint8_t_tlv *pkt =
      (struct qmi_generic_uint8_t_tlv *)(output + offset);
  pkt->id = id;
  pkt->len = 0x01;
  pkt->data = data;
  pkt = NULL;
  return 0;
}

uint16_t count_tlvs_in_message(uint8_t *bytes, size_t len) {
  uint16_t cur_byte;
  uint8_t *arr = (uint8_t *)bytes;
  uint16_t num_tlvs = 0;
  struct empty_tlv *this_tlv;
  logger(MSG_INFO, "%s: start\n", __func__);
  if (len < sizeof(struct encapsulated_qmi_packet) + 4) {
    logger(MSG_ERROR, "%s: Packet is too small \n", __func__);
    return 0;
  }

  cur_byte = sizeof(struct encapsulated_qmi_packet);
  while ((cur_byte) < len) {
    this_tlv = (struct empty_tlv *)(arr + cur_byte);
    num_tlvs++;
    cur_byte += le16toh(this_tlv->len) + sizeof(uint8_t) + sizeof(uint16_t);
    if (cur_byte <= 0 || cur_byte > len) {
      logger(MSG_ERROR, "Current byte is less than or exceeds size\n");
      arr = NULL;
      this_tlv = NULL;
      return num_tlvs;
    }
  }
  arr = NULL;
  this_tlv = NULL;
  return num_tlvs;
}
/* INTERNAL QMI CLIENT */

/*
 * Signals to the proxy thread that some internal service has a pending message
 */
void set_pending_message(uint8_t service) {
  logger(MSG_INFO, "%s: Pending QMI message for service %.2x\n", __func__,
         service);
  internal_qmi_client.services[service].has_pending_message = 1;
  internal_qmi_client.has_pending_message = 1;
}

/*
 * Attempts to read from /dev/smdcntl0
 */
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
  logger(MSG_INFO, "%s: Bytes read: %u\n", __func__, bytes_read);
  return bytes_read;
}

/*
 * Attempts to write a buffer to /dev/smdcntl0
 */
size_t write_to_qmi_port(void *buff, size_t bufsz) {
  size_t bytes_written = 0;

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
    return -ENOMEM;
  }
  memset(buff, 0, 1024);
  free(buff);
  return 0;
}

/*
 * Returns either 1 or 0 if there's a pending message to deliver
 * to the baseband
 */
uint8_t is_internal_qmi_message_pending() {
  if (internal_qmi_client.has_pending_message) {
    logger(MSG_INFO, "%s: Pending messages found\n", __func__);
    return 1;
  }
  return 0;
}

/*
 * Clears the buffer and the pending flag after sending a message
 * to the baseband
 */
void clear_message_for_service(uint8_t service) {
  if (service > QMI_SERVICES_LAST) {
    logger(MSG_ERROR, "%s: Invalid service %.2x!!\n", __func__, service);
    return;
  }

  if (internal_qmi_client.services[service].message == NULL) {
    logger(MSG_ERROR, "%s: Message for %.2x is already NULL!\n", __func__,
           service);
    return;
  }

  free(internal_qmi_client.services[service].message);
  internal_qmi_client.services[service].message = NULL;
  internal_qmi_client.services[service].message_len = 0;
  internal_qmi_client.services[service].has_pending_message = 0;
  logger(MSG_INFO, "%s: Message for %.2x cleared.\n", __func__, service);
}

/*
 * Set the transaction_id to 0 for a service
 */
void clear_current_transaction_id(uint8_t service) {
  if (service > QMI_SERVICES_LAST) {
    logger(MSG_ERROR, "%s: Invalid service %.2x!!\n", __func__, service);
    return;
  }
  internal_qmi_client.services[service].transaction_id = 0;
}

/*
 * Get last transaction_id for a service
 */
uint16_t get_transaction_id_for_service(uint8_t service) {
  if (service > QMI_SERVICES_LAST) {
    logger(MSG_ERROR, "%s: Invalid service %.2x!!\n", __func__, service);
    return 0;
  }
  return internal_qmi_client.services[service].transaction_id;
}

/*
 * Set a transaction ID for a service
 */
void set_transaction_id_for_service(uint8_t service, uint16_t transaction_id) {
  if (service > QMI_SERVICES_LAST) {
    logger(MSG_ERROR, "%s: Invalid service %.2x!!\n", __func__, service);
    return;
  }
  internal_qmi_client.services[service].transaction_id = transaction_id;
}

/*
 * Looks for pending internal messages and dispatches them from the service
 */
int send_pending_internal_qmi_messages() {
  uint8_t clear_pending_flag = 1;
  if (!internal_qmi_client.has_pending_message) {
    logger(MSG_WARN, "%s was called with no pending messages!\n", __func__);
    return -EINVAL;
  }
  for (uint8_t i = 0; i < QMI_SERVICES_LAST; i++) {
    if (internal_qmi_client.services[i].has_pending_message) {
      if (!internal_qmi_client.services[i].is_initialized) {
        /*
         * Try to allocate a new client, but don't send the pending message
         * just yet, but wait for next pass. This avoids hogging the port
         * if the client fails to be allocated and allows for the rest of the
         * stack to keep working
         */
        if (allocate_qmi_client(i) < 0) {
          logger(MSG_ERROR, "%s: Failed to allocate client: SVC %.2x\n",
                 __func__, i);
        } else {
          logger(MSG_INFO,
                 "%s: Requested allocation to svc %.2x. On next pass we'll "
                 "send the pending message\n",
                 __func__, i);
        }

      } else {
        switch (i) {          /* Just to add some debugging here */
        case QMI_SERVICE_WDS: // Wireless Data Service
          logger(MSG_INFO, "%s: Pending message for WDS\n", __func__);
          prepare_internal_pkt(internal_qmi_client.services[i].message,
                               internal_qmi_client.services[i].message_len);
          write_to_qmi_port(internal_qmi_client.services[i].message,
                            internal_qmi_client.services[i].message_len);
          clear_message_for_service(i);
          break;

        case QMI_SERVICE_DMS: // Device Management Service
          logger(MSG_INFO, "%s: Pending message for DMS\n", __func__);
          prepare_internal_pkt(internal_qmi_client.services[i].message,
                               internal_qmi_client.services[i].message_len);
          write_to_qmi_port(internal_qmi_client.services[i].message,
                            internal_qmi_client.services[i].message_len);
          clear_message_for_service(i);
          break;

        case QMI_SERVICE_VOICE: // Voice Service (9)
          logger(MSG_INFO, "%s: Pending message for Voice\n", __func__);
          prepare_internal_pkt(internal_qmi_client.services[i].message,
                               internal_qmi_client.services[i].message_len);
          write_to_qmi_port(internal_qmi_client.services[i].message,
                            internal_qmi_client.services[i].message_len);
          clear_message_for_service(i);
          break;

        case QMI_SERVICE_NAS: // Network Access Service
          logger(MSG_INFO, "%s: Pending message for NAS\n", __func__);
          prepare_internal_pkt(internal_qmi_client.services[i].message,
                               internal_qmi_client.services[i].message_len);
          write_to_qmi_port(internal_qmi_client.services[i].message,
                            internal_qmi_client.services[i].message_len);
          clear_message_for_service(i);
          break;

        default:
          logger(MSG_ERROR, "%s: Unknown service %.2x, discarding the flag\n",
                 __func__, i);
          internal_qmi_client.services[i].has_pending_message = 0;
          break;
        }
      }
    }
  }

  for (uint8_t i = 0; i < QMI_SERVICES_LAST; i++) {
    if (internal_qmi_client.services[i].has_pending_message == 1) {
      clear_pending_flag = 0;
    }
  }
  if (clear_pending_flag) {
    internal_qmi_client.has_pending_message = 0;
    logger(MSG_INFO, "%s: Pending flag cleared\n", __func__);
  }
  return 0;
}
/*
 * This is called from each client to add a message to the pool
 */
int add_pending_message(uint8_t service, uint8_t *buf, size_t buf_len) {
  uint8_t retries = 10;
  uint8_t *temporary_buffer = malloc(buf_len);
  if (service > QMI_SERVICES_LAST) {
    logger(MSG_ERROR, "%s: Invalid Service ID: %.2x\n", __func__);
    free(temporary_buffer);
    return -EINVAL;
  }
  memcpy(temporary_buffer, buf, buf_len);
  while (internal_qmi_client.services[service].has_pending_message &&
         retries > 0) {
    logger(MSG_WARN,
           "%s: The QMI Service %.2x already has something pending. We'll wait "
           "in line...\n",
           __func__, service);
    sleep(1);
    retries--;
  }

  if (!internal_qmi_client.services[service].has_pending_message) {
    internal_qmi_client.services[service].message = malloc(buf_len);
    internal_qmi_client.services[service].message_len = buf_len;
    memcpy(internal_qmi_client.services[service].message, temporary_buffer,
           buf_len);
    free(temporary_buffer);
    internal_qmi_client.services[service].has_pending_message = 1;
    internal_qmi_client.has_pending_message = 1;
    return 0;
  }

  logger(MSG_ERROR, "%s: Failed to add the pending message for service %.2x!\n",
         __func__, service);
  return -EINVAL;
}

int handle_incoming_qmi_control_message(uint8_t *buf, size_t buf_len) {

  switch (get_control_message_id(buf, buf_len)) {
  case CLIENT_REGISTER_REQ:
    if (buf_len >= sizeof(struct client_alloc_response)) {
      logger(MSG_INFO, "We may have a valid response, let's check it\n");
      struct client_alloc_response *response =
          (struct client_alloc_response *)buf;
      if (response->result.response == 0x00 &&
          response->result.result == 0x00) {
        logger(MSG_INFO,
               "%s: Yesss we allocated or client, instance ID: %.2x\n",
               __func__, response->instance.instance_id);
        internal_qmi_client.services[response->instance.service_id].service =
            response->instance.service_id;
        internal_qmi_client.services[response->instance.service_id].instance =
            response->instance.instance_id;
        internal_qmi_client.services[response->instance.service_id]
            .transaction_id = response->qmi.transaction_id;
        internal_qmi_client.services[response->instance.service_id]
            .is_initialized = 1;
      }
    } else {
      logger(MSG_ERROR, "%s: Invalid allocation response: %u bytes\n", __func__,
             buf_len);
    }
    break;
  case CLIENT_RELEASE_REQ:
    // Not implemented
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for CTL Service: %.4x\n", __func__,
           get_control_message_id(buf, buf_len));
    break;
  }
  return 0;
}

void dispatch_incoming_qmi_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Pending message delivery service\n", __func__);
  uint8_t service = get_qmux_service_id(buf, buf_len);
  uint16_t transaction_id = get_transaction_id(buf, buf_len);

  set_transaction_id_for_service(service, transaction_id + 1);
  switch (service) {
  case QMI_SERVICE_CONTROL:
    break;
  case QMI_SERVICE_DMS:
    handle_incoming_dms_message(buf, buf_len);
    break;
  case QMI_SERVICE_WDS:
    handle_incoming_wds_message(buf, buf_len);
    break;
  case QMI_SERVICE_VOICE:
    handle_incoming_voice_message(buf, buf_len);
    break;
  case QMI_SERVICE_NAS:
    handle_incoming_nas_message(buf, buf_len);
    break;
  default:
    logger(MSG_WARN, "%s: Unhandled target service: %.2x\n", __func__, service);
    break;
  }
  return;
}


/*
 * Sort of like the rmnet proxy, but for the internal SMD control port
 * The idea is to have a pseudo QMI client working inside openqti, to
 * avoid depending on communication going to the host, so this one should
 * be able to dispatch data from services, and also route incoming data
 * from QMI to the required service
 */
void *init_internal_qmi_client() {
  size_t buf_len;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  struct timeval tv;
  if (!internal_qmi_client.is_initialized) {
    internal_qmi_client.fd = open(INT_SMD_CNTL, O_RDWR);
    if (internal_qmi_client.fd < 0) {
      logger(MSG_ERROR, "Error opening %s!\n", INT_SMD_CNTL);
      return NULL;
      ;
    } else {
      logger(MSG_INFO, "%s: Opened internal SMD port\n", __func__);
      internal_qmi_client.is_initialized = 1;
    }
  }
  while (1) {
    FD_SET(internal_qmi_client.fd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if (is_internal_qmi_message_pending()) {
      send_pending_internal_qmi_messages();
    }
    select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(internal_qmi_client.fd, &readfds)) {
      buf_len = read(internal_qmi_client.fd, &buf, MAX_PACKET_SIZE);
      if (buf_len > sizeof(struct qmux_packet)) {
        if (get_qmux_service_id(buf, buf_len) == 0) {
          logger(MSG_INFO, "%s: New QMI Control Message of %i bytes\n",
                 __func__, buf_len);
          if (buf_len >
              (sizeof(struct qmux_packet) + sizeof(struct ctl_qmi_packet))) {
              handle_incoming_qmi_control_message(buf, buf_len);
          } else {
            logger(MSG_WARN, "%s: Size is too small!\n", __func__);
          }
        } else {
          logger(MSG_INFO, "%s: New QMI Message of %i bytes\n", __func__,
                 buf_len);
          if (buf_len >
              (sizeof(struct qmux_packet) + sizeof(struct qmi_packet))) {
// Too much noise with this printing everything
//            pretty_print_qmi_pkt("Baseband --> Host", buf, buf_len);
            dispatch_incoming_qmi_message(buf, buf_len);
          } else {
            logger(MSG_WARN, "%s: Size is too small!\n", __func__);
          }
        }
      }
    }


      /* Demo: put code below */
    }

    return NULL;
  }

  uint8_t is_internal_qmi_client_ready() {
    return internal_qmi_client.is_initialized;
  }

/*
 * We kickstart connections to all services from here
 */
void *start_service_initialization_thread() {
  do {
    logger(MSG_INFO, "%s: Waiting for QMI client ready...\n", __func__);
    sleep(5);
  } while (!is_internal_qmi_client_ready());

  logger(MSG_INFO, "%s: QMI Client appears ready, gather modem info\n",
         __func__);

  register_to_nas_service();
  register_to_voice_service();
  dms_get_modem_info();
  return NULL;
}