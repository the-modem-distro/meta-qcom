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
#include "../inc/dms.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/qmi.h"

#define DMS_MODEM_INFO_MAX_STR_LEN 256
struct {
  char modem_revision[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_serial_num[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_manufacturer[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_hw_rev[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_model[DMS_MODEM_INFO_MAX_STR_LEN];
} dms_runtime;

void reset_dms_runtime() {
  memset(dms_runtime.modem_revision, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_serial_num, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_manufacturer, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_hw_rev, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_model, 0, DMS_MODEM_INFO_MAX_STR_LEN);
}

const char *get_dms_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(dms_svc_commands) / sizeof(dms_svc_commands[0])); i++) {
    if (dms_svc_commands[i].id == msgid) {
      return dms_svc_commands[i].cmd;
    }
  }
  return "DMS: Unknown command\n";
}


int dms_request_model() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  /* We don't worry about instances or transaction IDs, we delegate that to the
   * client code
   */

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       DMS_GET_MODEL) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}


int dms_request_serial_number() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  /* We don't worry about instances or transaction IDs, we delegate that to the
   * client code
   */

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       DMS_GET_SERIAL_NUM) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int dms_request_revision() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  /* We don't worry about instances or transaction IDs, we delegate that to the
   * client code
   */

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       DMS_GET_REVISION) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}


int dms_request_hw_rev() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  /* We don't worry about instances or transaction IDs, we delegate that to the
   * client code
   */

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       DMS_GET_HARDWARE_REVISION) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}


void *dms_get_modem_info() {
  do {
    sleep(5);
  } while (!is_internal_qmi_client_ready());

  logger(MSG_INFO, "%s: QMI Client appears ready, gather modem info\n",
         __func__);
  dms_request_model();
  dms_request_hw_rev();
  dms_request_revision();
  dms_request_serial_number();

  return NULL;
}

/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_dms_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);

  switch (get_qmi_message_id(buf, buf_len)) {
  case DMS_RESET:
  case DMS_SET_EVENT_REPORT:
  case DMS_REGISTER:
  case DMS_GET_MODEL:
  case DMS_GET_REVISION:
  case DMS_GET_SERIAL_NUM:
  case DMS_GET_TIME:
  case DMS_SET_TIME:
  case DMS_GET_HARDWARE_REVISION:
    logger(MSG_WARN, "%s: Incoming message from QMI client: %s\n", __func__,
           get_dms_command(get_qmi_message_id(buf, buf_len)));
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for DMS: %.4x\n", __func__,
           get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}