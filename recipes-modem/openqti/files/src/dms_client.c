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
  char modem_hw_rev[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_model[DMS_MODEM_INFO_MAX_STR_LEN];
} dms_runtime;

void reset_dms_runtime() {
  memset(dms_runtime.modem_revision, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_serial_num, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_hw_rev, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_model, 0, DMS_MODEM_INFO_MAX_STR_LEN);
}

const char *dms_get_modem_revision() {
    if (dms_runtime.modem_revision[0] == 0) {
        return "[DMS not ready]";
    }
    return dms_runtime.modem_revision;
}
const char *dms_get_modem_modem_serial_num() {
    if (dms_runtime.modem_serial_num[0] == 0) {
        return "[DMS not ready]";
    }
    return dms_runtime.modem_serial_num;
}

const char *dms_get_modem_modem_hw_rev() {
    if (dms_runtime.modem_hw_rev[0] == 0) {
        return "[DMS not ready]";
    }
    return dms_runtime.modem_hw_rev;
}
const char *dms_get_modem_modem_model() {
    if (dms_runtime.modem_model[0] == 0) {
        return "[DMS not ready]";
    }
    return dms_runtime.modem_model;
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
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_GET_MODEL) < 0) {
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
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_GET_SERIAL_NUM) < 0) {
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
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_GET_REVISION) < 0) {
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
  reset_dms_runtime();
  do {
    logger(MSG_INFO, "DMS: Wait!\n");
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
  case DMS_GET_MODEL:
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
        int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
        if (offset > 0) {
            struct qmi_generic_uch_arr *resp = (struct qmi_generic_uch_arr*)(buf + offset);
            if (resp->len < DMS_MODEM_INFO_MAX_STR_LEN) {
                memcpy(dms_runtime.modem_model, resp->data, resp->len);
            }
        }
    }
    break;
  case DMS_GET_REVISION:
     if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
        int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
        if (offset > 0) {
            struct qmi_generic_uch_arr *resp = (struct qmi_generic_uch_arr*)(buf + offset);
            if (resp->len < DMS_MODEM_INFO_MAX_STR_LEN) {
                strncpy(dms_runtime.modem_revision, (char*)resp->data, resp->len);
            }
        }
    }
    break;
  case DMS_GET_SERIAL_NUM:
     if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
        int offset = get_tlv_offset_by_id(buf, buf_len, 0x12);
        if (offset > 0) {
            struct qmi_generic_uch_arr *resp = (struct qmi_generic_uch_arr*)(buf + offset);
            if (resp->len < DMS_MODEM_INFO_MAX_STR_LEN) {
                memcpy(dms_runtime.modem_serial_num, resp->data, resp->len);
            }
        }
    }
    break;
  case DMS_GET_HARDWARE_REVISION:
     if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
        int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
        if (offset > 0) {
            struct qmi_generic_uch_arr *resp = (struct qmi_generic_uch_arr*)(buf + offset);
            if (resp->len <= DMS_MODEM_INFO_MAX_STR_LEN) {
                memcpy(dms_runtime.modem_hw_rev, resp->data, resp->len);
            }
        }
    }
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for DMS: %.4x\n", __func__,
           get_qmi_message_id(buf, buf_len));
    break;
  }

  logger(MSG_INFO, "%s: Current data:\n"
                    "Revision: %s\n"
                    "Serial Number: %s\n"
                    "HW Revision: %s\n"
                    "Model: %s\n",
                    __func__,
                    dms_get_modem_revision(),
                    dms_get_modem_modem_serial_num(),
                    dms_get_modem_modem_hw_rev(),
                    dms_get_modem_modem_model() );

  return 0;
}