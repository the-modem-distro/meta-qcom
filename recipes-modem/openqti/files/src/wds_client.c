// SPDX-License-Identifier: MIT

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/cell.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/qmi.h"
#include "../inc/wds.h"

struct {
  uint8_t service_id;      // 0x01: Wireless Data Service
  uint8_t instance_id;     // Dynamically allocated
  uint16_t transaction_id; // Current transaction ID
  uint8_t is_initialized;
  uint8_t mux_ready;
  int internal_smd_fd;
  int pdp_session_handle; // we need this to stop the connection later
} wds_runtime;

void reset_wds_runtime() {
  wds_runtime.service_id = 0x01;
  wds_runtime.instance_id = 0x00;
  wds_runtime.transaction_id = 0;
  wds_runtime.is_initialized = 0;
  wds_runtime.mux_ready = 0;
  wds_runtime.internal_smd_fd = -1;
  wds_runtime.pdp_session_handle = -1;
}

uint8_t is_wds_initialized() { return wds_runtime.is_initialized; }

size_t read_from_port(void *buff, size_t max_sz) {
  size_t bytes_read = 0;
  fd_set readfds;
  struct timeval tv;

  memset(buff, 0, max_sz);
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  FD_SET(wds_runtime.internal_smd_fd, &readfds);
  select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(wds_runtime.internal_smd_fd, &readfds)) {
    logger(MSG_INFO, "%s: Is set!\n", __func__);
    bytes_read = read(wds_runtime.internal_smd_fd, buff, max_sz);
  }
  set_log_level(0);
  dump_pkt_raw(buff, bytes_read);
  set_log_level(1);
  logger(MSG_INFO, "%s: Bytes read: %u\n", __func__, bytes_read);
  return bytes_read;
}

size_t write_to_port(void *buff, size_t bufsz) {
  size_t bytes_written = 0;
  set_log_level(0);
  dump_pkt_raw(buff, bufsz);
  set_log_level(1);
  bytes_written = write(wds_runtime.internal_smd_fd, buff, bufsz);
  if (bytes_written != bufsz) {
    logger(MSG_WARN,
           "%s: Heey we wrote %u bytes from %u that we were supposed to...\n",
           __func__, bytes_written, bufsz);
  }
  return bytes_written;
}

int allocate_internal_wds_client() {
  uint8_t *buff = malloc(1024 * sizeof(uint8_t));
  struct client_alloc_request *request = NULL;
  size_t bytes = 0;
  int ret = 0;
  wds_runtime.internal_smd_fd = open(INT_SMD_CNTL, O_RDWR);
  if (wds_runtime.internal_smd_fd < 0) {
    logger(MSG_ERROR, "Error opening %s!\n", INT_SMD_CNTL);
    return -EINVAL;
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
  request->service.service_id = 0x01; // 0x01 -> WDS
  if (write_to_port((void *)request, sizeof(struct client_alloc_request)) <
      sizeof(struct client_alloc_request)) {
    logger(MSG_ERROR, "Allocation failed\n");
    free(buff);
    return -ENOMEM;
  }
  memset(buff, 0, 1024);
  bytes = read_from_port(buff, 1024);
  if (bytes >= sizeof(struct client_alloc_response)) {
    logger(MSG_INFO, "We may have a valid response, let's check it\n");
    struct client_alloc_response *response =
        (struct client_alloc_response *)buff;
      if (response->result.response == 0x00 &&
          response->result.result == 0x00) {
        logger(MSG_INFO, "Yesss we allocated or client, instance ID: %.2x\n",
               response->instance.instance_id);
        wds_runtime.service_id = response->instance.service_id;
        wds_runtime.instance_id = response->instance.instance_id;
        wds_runtime.transaction_id = response->qmi.transaction_id;
        wds_runtime.is_initialized = 1;
      }
    /*
      If we have a 0x02 and a 0x01, and sizes are right, check indication result
    */
    //  int offset = get_tlv_offset_by_id(buff, bytes, 0x02);
    //  logger(MSG_INFO, "TLV 0x02 offset at %.2x\n", offset);
  } else {
    logger(MSG_ERROR, "%s: Invalid allocation response: %u bytes\n", __func__,
           bytes);
  }
  free(buff);
  return ret;
}

int wds_bind_mux_data_port() {
  struct wds_bind_mux_data_port_request *request = NULL;
  int ret = 0;
  uint16_t qmi_err = 0;
  size_t bytes = 0;
  uint8_t *buff = malloc(1024 * sizeof(uint8_t));

  if (is_internal_connect_enabled()) {
    logger(MSG_INFO, "%s: Bind mux data port request!\n", __func__);
  } else {
    logger(MSG_INFO, "%s: Internal connectivity support is not enabled\n",
           __func__);
    return -EINVAL;
  }
  wds_runtime.transaction_id++;
  request = (struct wds_bind_mux_data_port_request *)calloc(
      1, sizeof(struct wds_bind_mux_data_port_request));
  request->qmux.version = 0x01;
  request->qmux.packet_length =
      sizeof(struct wds_bind_mux_data_port_request) - sizeof(uint8_t);
  request->qmux.control = 0x00;
  request->qmux.service = wds_runtime.service_id;
  request->qmux.instance_id = wds_runtime.instance_id;

  request->qmi.ctlid = 0x00;
  request->qmi.transaction_id = wds_runtime.transaction_id;
  request->qmi.msgid = WDS_BIND_MUX_DATA_PORT;
  request->qmi.length = sizeof(struct wds_bind_mux_data_port_request) -
                        sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  request->peripheral_id.type = 0x10;
  request->peripheral_id.len = 0x08;
  request->peripheral_id.ep_type = htole32(DATA_EP_TYPE_BAM_DMUX);
  request->peripheral_id.interface_id = htole32(0);

  request->mux.type = 0x11;
  request->mux.len = 0x01;
  request->mux.mux_id = 42;

  bytes = write_to_port(request, sizeof(struct wds_bind_mux_data_port_request));
  logger(MSG_WARN, "%s: Bytes written %u\n", __func__, bytes);
  bytes = read_from_port(buff, 1024);
  logger(MSG_WARN, "%s: Bytes read from buff: %u\n", __func__, bytes);
  qmi_err = did_qmi_op_fail(buff, bytes);
  if (qmi_err == QMI_RESULT_FAILURE) {
    logger(MSG_ERROR, "%s failed\n", __func__);
    ret = -EFAULT;
  } else  if (qmi_err == QMI_RESULT_SUCCESS) {
    logger(MSG_INFO, "%s succeeded\n", __func__);
    wds_runtime.mux_ready = 1;
  } else if (qmi_err == QMI_RESULT_UNKNOWN) {
    logger(MSG_ERROR, "%s: QMI message didn't have an indication\n", __func__);
    ret = -EINVAL;
  }

  free(request);
  free(buff);
  return ret;
}

int wds_attempt_to_connect() {
  struct wds_start_network *request = NULL;
  size_t bytes = 0;
  uint8_t *buff = malloc(1024 * sizeof(uint8_t));
  if (is_internal_connect_enabled()) {
    logger(MSG_INFO, "%s: Autoconnect request!\n", __func__);
  } else {
    logger(MSG_INFO, "%s: Internal connectivity support is not enabled\n",
           __func__);
    return -EINVAL;
  }
  if (!wds_runtime.mux_ready)
    wds_bind_mux_data_port();
  else
   logger(MSG_INFO, "Mux was already setup\n");

  wds_runtime.transaction_id++;
  request =
      (struct wds_start_network *)calloc(1, sizeof(struct wds_start_network));
  request->qmux.version = 0x01;
  request->qmux.packet_length =
      sizeof(struct wds_start_network) - sizeof(uint8_t);
  request->qmux.control = 0x00;
  request->qmux.service = wds_runtime.service_id;
  request->qmux.instance_id = wds_runtime.instance_id;

  request->qmi.ctlid = 0x00;
  request->qmi.transaction_id = wds_runtime.transaction_id;
  request->qmi.msgid = WDS_START_NETWORK_IF;
  request->qmi.length = sizeof(struct wds_start_network) -
                        sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  request->apn.type = 0x14;
  request->apn.len = 8;
  strcpy(request->apn.apn, DEFAULT_APN_NAME);

  request->apn_type.type = 0x38;
  request->apn_type.len = 0x04;
  request->apn_type.value = htole32(WDS_APN_TYPE_INTERNET);

  request->ip_family.type = 0x19;
  request->ip_family.len = 0x01;
  request->ip_family.value = 4;

  request->profile.type = 0x31;
  request->profile.len = 0x01;
  request->profile.value = 2; // Let's try this one

  request->call_type.type = 0x35;
  request->call_type.len = 0x01;
  request->call_type.value = 1; // Let's try with embedded

  request->autoconnect.type = 0x33;
  request->autoconnect.len = 0x01;
  request->autoconnect.value = 0x01; // ON

  request->roaming_lock.type = 0x39;
  request->roaming_lock.len = 0x01;
  request->roaming_lock.value = 0x00; // OFF

  bytes = write_to_port(request, sizeof(struct wds_start_network));
  logger(MSG_WARN, "%s: Bytes written %u\n", __func__, bytes);
  bytes = read_from_port(buff, 1024);
  logger(MSG_WARN, "%s: Bytes read from buff: %u\n", __func__, bytes);
  if (did_qmi_op_fail(buff, bytes)) {
    logger(MSG_ERROR, "%s failed\n", __func__);
  }
  free(request);
  free(buff);
  return 0;
}


int wds_attempt_to_connect_slim() {
  struct wds_start_network_autoconnect *request = NULL;
  size_t bytes = 0;
  uint8_t *buff = malloc(1024 * sizeof(uint8_t));
  if (is_internal_connect_enabled()) {
    logger(MSG_INFO, "%s: Autoconnect request!\n", __func__);
  } else {
    logger(MSG_INFO, "%s: Internal connectivity support is not enabled\n",
           __func__);
    return -EINVAL;
  }
  if (!wds_runtime.mux_ready)
    wds_bind_mux_data_port();
  else
   logger(MSG_INFO, "Mux was already setup\n");

  wds_runtime.transaction_id++;
  request =
      (struct wds_start_network_autoconnect *)calloc(1, sizeof(struct wds_start_network));
  request->qmux.version = 0x01;
  request->qmux.packet_length =
      sizeof(struct wds_start_network_autoconnect) - sizeof(uint8_t);
  request->qmux.control = 0x00;
  request->qmux.service = wds_runtime.service_id;
  request->qmux.instance_id = wds_runtime.instance_id;

  request->qmi.ctlid = 0x00;
  request->qmi.transaction_id = wds_runtime.transaction_id;
  request->qmi.msgid = WDS_START_NETWORK_IF;
  request->qmi.length = sizeof(struct wds_start_network_autoconnect) -
                        sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  request->autoconnect.type = 0x33;
  request->autoconnect.len = 0x01;
  request->autoconnect.value = 0x01; // ON


  bytes = write_to_port(request, sizeof(struct wds_start_network_autoconnect));
  logger(MSG_WARN, "%s: Bytes written %u\n", __func__, bytes);
  bytes = read_from_port(buff, 1024);
  logger(MSG_WARN, "%s: Bytes read from buff: %u\n", __func__, bytes);
  if (did_qmi_op_fail(buff, bytes)) {
    logger(MSG_ERROR, "%s failed\n", __func__);
  }
  free(request);
  free(buff);
  return 0;
}

void *init_internal_networking() {
  if (!is_internal_connect_enabled()) {
    logger(MSG_WARN, "%s: Internal networking is disabled\n", __func__);
    return NULL;
  }

  while (!get_network_type()) {
    logger(MSG_INFO, "[%s] Waiting for network to be ready...\n", __func__);
    sleep(10);
  }

  logger(MSG_INFO, "%s: Network is ready!\n", __func__);
  if (!wds_runtime.is_initialized) {
    logger(MSG_INFO,
           "[%s] Attempting to initialize a WDS client...\n", __func__);
    allocate_internal_wds_client();
  } else {
    logger(MSG_INFO, "[%s] Client already initialized, ID %.2x\n",
           __func__, wds_runtime.instance_id);
  }

  if (wds_runtime.is_initialized) {
    logger(MSG_INFO, "[%s] Client ready, attempting to connect!\n",
           __func__);
    wds_attempt_to_connect();
  }


  if (wds_runtime.is_initialized) {
    logger(MSG_INFO, "[%s] ATTEMPT 2 attempting to connect!\n",
           __func__);
    wds_attempt_to_connect_slim();
  }

  
  return NULL;
}
