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

struct fw_parts {
  char primary[DMS_MODEM_INFO_MAX_STR_LEN];
  char sbl[DMS_MODEM_INFO_MAX_STR_LEN];
  char tz[DMS_MODEM_INFO_MAX_STR_LEN];
  char tz_sec[DMS_MODEM_INFO_MAX_STR_LEN];
  char rpm[DMS_MODEM_INFO_MAX_STR_LEN];
  char aboot[DMS_MODEM_INFO_MAX_STR_LEN];
  char apps[DMS_MODEM_INFO_MAX_STR_LEN];
  char mpss[DMS_MODEM_INFO_MAX_STR_LEN];
  char adsp[DMS_MODEM_INFO_MAX_STR_LEN];
};

struct {
  char modem_revision[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_serial_num[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_hw_rev[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_model[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_fw_pref[DMS_MODEM_INFO_MAX_STR_LEN];
  char modem_sw_ver[DMS_MODEM_INFO_MAX_STR_LEN];
  struct fw_parts firmware_sections;
} dms_runtime;

void reset_dms_runtime() {
  memset(dms_runtime.modem_revision, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_serial_num, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_hw_rev, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_model, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_fw_pref, 0, DMS_MODEM_INFO_MAX_STR_LEN);
  memset(dms_runtime.modem_sw_ver, 0, DMS_MODEM_INFO_MAX_STR_LEN);
}

const char *dms_get_modem_revision() {
  return (dms_runtime.modem_revision[0] == 0 ? "[Not set]"
                                             : dms_runtime.modem_revision);
}
const char *dms_get_modem_modem_serial_num() {
  return (dms_runtime.modem_serial_num[0] == 0 ? "[Not set]"
                                               : dms_runtime.modem_serial_num);
}

const char *dms_get_modem_modem_hw_rev() {
  return (dms_runtime.modem_hw_rev[0] == 0 ? "[Not set]"
                                           : dms_runtime.modem_hw_rev);
}

const char *dms_get_modem_modem_model() {
  return (dms_runtime.modem_model[0] == 0 ? "[Not set]"
                                          : dms_runtime.modem_model);
}

const char *dms_get_modem_modem_fw_pref() {
  return (dms_runtime.modem_fw_pref[0] == 0 ? "[Not set]"
                                            : dms_runtime.modem_fw_pref);
}

const char *dms_get_modem_modem_sw_ver() {
  return (dms_runtime.modem_sw_ver[0] == 0 ? "[Not set]"
                                           : dms_runtime.modem_sw_ver);
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
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_GET_IDS) < 0) {
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

int dms_register_to_events() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (1 * sizeof(struct qmi_generic_uint8_t_tlv));
  uint8_t *pkt = malloc(pkt_len);
  uint8_t tlvs[] = {EVENT_TLV_POWER_STATE,  EVENT_TLV_PIN_STATE,
                    EVENT_ACTIVATION_STATE, EVENT_OP_MODE,
                    EVENT_UIM_STATE,        EVENT_AIRPLANE_MODE_STATE};

  for (int i = 0; i < 6; i++) {
    memset(pkt, 0, pkt_len);

    /* We don't worry about instances or transaction IDs, we delegate that to
     * the client code
     */

    if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
      logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_SET_EVENT_REPORT) <
        0) {
      logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
    if (build_u8_tlv(pkt, pkt_len, curr_offset, tlvs[i], 1) < 0) {
      logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
      free(pkt);
      return -EINVAL;
    }

    add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);
  }

  free(pkt);
  return 0;
}

int dms_request_sw_ver() {
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
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_GET_SOFTWARE_VERSION) <
      0) {
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
  dms_register_to_events();
  dms_request_sw_ver();
  logger(MSG_INFO, "%s done!\n", __func__);
  return NULL;
}

void parse_dms_event_report(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO,
         "************************* %s: New event report! If only I would have "
         "implemented it...\n",
         __func__);
  did_qmi_op_fail(buf, buf_len);
}
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_dms_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);

  switch (get_qmi_message_id(buf, buf_len)) {
  case DMS_EVENT_REPORT:
    parse_dms_event_report(buf, buf_len);
    break;
  case DMS_GET_MODEL:
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
      int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
      if (offset > 0) {
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
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
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
        if (resp->len < DMS_MODEM_INFO_MAX_STR_LEN) {
          strncpy(dms_runtime.modem_revision, (char *)resp->data, resp->len);
        }
      }
    }
    break;
  case DMS_GET_IDS:
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
      int offset = get_tlv_offset_by_id(buf, buf_len, 0x12);
      if (offset > 0) {
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
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
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
        if (resp->len <= DMS_MODEM_INFO_MAX_STR_LEN) {
          memcpy(dms_runtime.modem_hw_rev, resp->data, resp->len);
        }
      }
    }
    break;
  case DMS_GET_STORED_IMAGE_INFO:
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
      int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
      if (offset > 0) {
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
        if (resp->len <= DMS_MODEM_INFO_MAX_STR_LEN) {
          memcpy(dms_runtime.modem_fw_pref, resp->data, resp->len);
        }
      }
    }
    break;
  case DMS_GET_SOFTWARE_VERSION:
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
      int offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
      if (offset > 0) {
        struct qmi_generic_uch_arr *resp =
            (struct qmi_generic_uch_arr *)(buf + offset);
        if (resp->len <= DMS_MODEM_INFO_MAX_STR_LEN) {
          strncpy(dms_runtime.firmware_sections.primary, (char *)resp->data,
                  resp->len);
        }
      }
      /* Parse multistring size here */
      offset = get_tlv_offset_by_id(buf, buf_len, 0x10);
      if (offset > 0) {
        struct dms_fw_version_strings_info *resp =
            (struct dms_fw_version_strings_info *)(buf + offset);
        int curr_sw_ver_offset = 0;
        for (int i = 0; i < resp->version_count; i++) {
          if (curr_sw_ver_offset + offset > buf_len) {
            logger(MSG_ERROR,
                   "%s: Expected offset is bigger than message size\n");
            return -ENOMEM;
          }
          struct dms_firmware_rel_string *rel =
              (struct dms_firmware_rel_string
                   *)(buf + (curr_sw_ver_offset + offset +
                             (sizeof(struct dms_fw_version_strings_info))));
          curr_sw_ver_offset +=
              rel->version_length + sizeof(uint8_t) + sizeof(uint32_t);
          if (rel->version_length > 0) {
            logger(MSG_INFO, "%s: Version For FW %i: %s\n", __func__,
                   rel->section, rel->fw_version);
            switch (rel->section) {
            case FIRMWARE_STR_SBL:
              strncpy(dms_runtime.firmware_sections.sbl,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_TZ:
              strncpy(dms_runtime.firmware_sections.tz, (char *)rel->fw_version,
                      rel->version_length);
              break;
            case FIRMWARE_STR_TZ_SEC:
              strncpy(dms_runtime.firmware_sections.tz_sec,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_RPM:
              strncpy(dms_runtime.firmware_sections.rpm,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_BOOTLOADER:
              strncpy(dms_runtime.firmware_sections.aboot,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_APPS:
              strncpy(dms_runtime.firmware_sections.apps,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_MPSS:
              strncpy(dms_runtime.firmware_sections.mpss,
                      (char *)rel->fw_version, rel->version_length);
              break;
            case FIRMWARE_STR_ADSP:
              strncpy(dms_runtime.firmware_sections.adsp,
                      (char *)rel->fw_version, rel->version_length);
              break;
            default:
              logger(MSG_WARN,
                     "%s: Unknwon firmware piece 0x%x with version %s\n",
                     rel->section, rel->fw_version);
              break;
            }
          }
        }
      }
    }
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for DMS: %.4x\n", __func__,
           get_qmi_message_id(buf, buf_len));
    break;
  }

  logger(MSG_INFO,
         "%s: Current data:\n"
         "\tRevision: %s\n"
         "\tSerial Number: %s\n"
         "\tHW Revision: %s\n"
         "\tModel: %s\n"
         "\tFW Preference: %s\n"
         "\tSW Version: %s\n",
         __func__, dms_get_modem_revision(), dms_get_modem_modem_serial_num(),
         dms_get_modem_modem_hw_rev(), dms_get_modem_modem_model(),
         dms_get_modem_modem_fw_pref(), dms_get_modem_modem_sw_ver());

  return 0;
}