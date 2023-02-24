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

#include "config.h"
#include "devices.h"
#include "dms.h"
#include "ipc.h"
#include "logger.h"
#include "qmi.h"

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
  char modem_sw_ver[DMS_MODEM_INFO_MAX_STR_LEN];
  struct fw_parts firmware_sections;
  uint8_t sim_lock_status;
} dms_runtime;

void print_modem_information() {
  logger(MSG_INFO, "Queried modem info:\n");
  logger(MSG_INFO, "\t Model: %s\n", dms_runtime.modem_model);
  logger(MSG_DEBUG, "\t Serial Number: %s\n", dms_runtime.modem_serial_num);
  logger(MSG_INFO, "\t HW Revision: %s\n", dms_runtime.modem_hw_rev);
  logger(MSG_INFO, "\t Revision: %s\n", dms_runtime.modem_revision);
  logger(MSG_INFO, "\t SW Version: %s\n", dms_runtime.modem_sw_ver);
  logger(MSG_INFO, "ADSP Section versions:\n");
  logger(MSG_INFO, "\t Primary: %s\n", dms_runtime.firmware_sections.primary);
  logger(MSG_INFO, "\t SBL: %s\n", dms_runtime.firmware_sections.sbl);
  logger(MSG_INFO, "\t TrustZone Kernel: %s\n",
         dms_runtime.firmware_sections.tz);
  logger(MSG_DEBUG, "\t TrustZone Kernel (sec): %s\n",
         dms_runtime.firmware_sections.tz_sec);
  logger(MSG_INFO, "\t Resource/Power Manager: %s\n",
         dms_runtime.firmware_sections.rpm);
  logger(MSG_DEBUG, "\t Aboot: %s\n", dms_runtime.firmware_sections.aboot);
  logger(MSG_DEBUG, "\t Apps: %s\n", dms_runtime.firmware_sections.apps);
  logger(MSG_INFO, "\t MPSS: %s\n", dms_runtime.firmware_sections.mpss);
  logger(MSG_DEBUG, "\t ADSP: %s\n", dms_runtime.firmware_sections.adsp);
}

uint8_t get_sim_lock_state() {
  return dms_runtime.sim_lock_status;
}
const char *dms_get_modem_revision() { return dms_runtime.modem_revision; }
const char *dms_get_modem_modem_serial_num() {
  return dms_runtime.modem_serial_num;
}

const char *dms_get_modem_modem_hw_rev() { return dms_runtime.modem_hw_rev; }

const char *dms_get_modem_modem_model() { return dms_runtime.modem_model; }

const char *dms_get_modem_modem_sw_ver() { return dms_runtime.modem_sw_ver; }

const char *get_dms_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(dms_svc_commands) / sizeof(dms_svc_commands[0])); i++) {
    if (dms_svc_commands[i].id == msgid) {
      return dms_svc_commands[i].cmd;
    }
  }
  return "DMS: Unknown command\n";
}

const char *get_sim_unlock_state_text(uint8_t state) {
  switch (state) {
    case 1:
      return "PIN enabled, SIM locked";
    case 2:
      return "PIN enabled, SIM unlocked";
    case 3:
      return "PIN disabled, SIM unlocked";
  }
  return "Unknown state";
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
  uint8_t tlvs[] = {DMS_EVENT_POWER_STATE, DMS_EVENT_PIN2_STATUS,
                    DMS_EVENT_ACT_STATE, DMS_EVENT_OPERATING_MODE};

  /* So, if we try to subscribe to everything at the same time and
   * we fail we don't subscribe to anything at all. So I iterate
   * through the tlvs and attempt to register to receive indications
   * for each one. If one fails to register the others will still work
   */
  for (int i = 0; i < 4; i++) {
    memset(pkt, 0, pkt_len);
    if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
      logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_EVENT_REPORT) < 0) {
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

int dms_register_to_indications() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (3 * sizeof(struct qmi_generic_uint8_t_tlv));
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_DMS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, DMS_REGISTER_INDICATIONS) <
      0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  if (build_u8_tlv(pkt, pkt_len, curr_offset,
                   EVENT_INDICATION_TLV_POWER_STATE_MODE_STATUS, 1) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  if (build_u8_tlv(pkt, pkt_len, curr_offset,
                   EVENT_INDICATION_TLV_POWER_STATE_MODE_CONFIG, 1) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  if (build_u8_tlv(pkt, pkt_len, curr_offset,
                   EVENT_INDICATION_TLV_IMS_CAPABILITY, 1) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_DMS, (uint8_t *)pkt, pkt_len);

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

void parse_dms_event_report(uint8_t *buf, size_t buf_len) {
  /* An event report response has an result TLV, while
   * an indication does not. I can use this to discern the response
   * from the baseband
   */
  int result_tlv = get_tlv_offset_by_id(buf, buf_len, 0x02);
  if (result_tlv > 0) {
    logger(MSG_DEBUG, "%s: This is a response to one of our requests\n",
           __func__);
    if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
      logger(MSG_INFO, "%s: Registered to event reports!\n",
             __func__);
    } else {
      logger(MSG_WARN, "%s: DMS Event report operation returned an error\n",
             __func__);
    }
  } else {
    logger(MSG_DEBUG, "%s: This is an unsolicited indication from the modem\n",
           __func__);
    uint8_t available_events[] = {
        DMS_EVENT_POWER_STATE, DMS_EVENT_PIN_STATUS,     DMS_EVENT_PIN2_STATUS,
        DMS_EVENT_ACT_STATE,   DMS_EVENT_OPERATING_MODE, DMS_EVENT_CAPABILITY};
    uint16_t tlvs = count_tlvs_in_message(buf, buf_len);
    logger(MSG_INFO, "%s: Found %u events in this message\n", __func__, tlvs);
    for (uint8_t i = 0; i < 6; i++) {
      int offset = get_tlv_offset_by_id(buf, buf_len, available_events[i]);
      if (offset > 0) {
        logger(MSG_INFO, "%s: TLV %.2x found at offset %.2x\n", __func__,
               available_events[i], offset);
        switch (available_events[i]) {
        case DMS_EVENT_POWER_STATE:
          logger(MSG_INFO, "%s: DMS_EVENT_POWER_STATE\n", __func__);
          break;
        case DMS_EVENT_PIN_STATUS:
          logger(MSG_INFO, "%s: DMS_EVENT_PIN_STATUS\n", __func__);
          struct dms_event_pin_status_info *pinstate =
              (struct dms_event_pin_status_info *)(buf + offset);
          dms_runtime.sim_lock_status = pinstate->pin_state;
          logger(MSG_INFO,
                 "%s: Pin status report:\n"
                 "\t State: %s (%.2x)\n"
                 "\t PIN retries left: %u\n"
                 "\t PUK retries left: %u\n",
                 __func__, get_sim_unlock_state_text(pinstate->pin_state), pinstate->pin_state, pinstate->pin_retries_left,
                 pinstate->puk_retries_left);
          break;
        case DMS_EVENT_PIN2_STATUS:
          logger(MSG_INFO, "%s: DMS_EVENT_PIN2_STATUS\n", __func__);
          break;
        case DMS_EVENT_ACT_STATE:
          logger(MSG_INFO, "%s: DMS_EVENT_ACT_STATE\n", __func__);
          break;
        case DMS_EVENT_OPERATING_MODE:
          logger(MSG_INFO, "%s: DMS_EVENT_OPERATING_MODE\n", __func__);
          break;
        case DMS_EVENT_CAPABILITY:
          logger(MSG_INFO, "%s: DMS_EVENT_CAPABILITY\n", __func__);
          break;
        }
      }
    }
  }
}

void check_and_set_fw_string(uint8_t *buf, size_t buf_len, uint8_t tlvid,
                             char *target_str) {
  if (did_qmi_op_fail(buf, buf_len) == QMI_RESULT_SUCCESS) {
    int offset = get_tlv_offset_by_id(buf, buf_len, tlvid);
    if (offset > 0) {
      struct qmi_generic_uch_arr *resp =
          (struct qmi_generic_uch_arr *)(buf + offset);
      if (resp->len < DMS_MODEM_INFO_MAX_STR_LEN) {
        memcpy(target_str, resp->data, resp->len);
      }
    }
  }
}

/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_dms_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);
  
  #ifdef DEBUG_DMS
  pretty_print_qmi_pkt("DMS: Baseband --> Host", buf, buf_len);
  #endif 

  switch (get_qmi_message_id(buf, buf_len)) {
  case DMS_EVENT_REPORT:
    parse_dms_event_report(buf, buf_len);
    break;
  case DMS_REGISTER_INDICATIONS:
    logger(MSG_INFO, "%s: Register indication response\n", __func__);
    break;
  case DMS_GET_PSM_INDICATION:
    logger(MSG_INFO, "%s: Power Save mode indication\n", __func__);
    break;
  case DMS_GET_PSM_CONFIG_CHANGE_INDICATION:
    logger(MSG_INFO, "%s: Power Save mode config change indication\n",
           __func__);
    break;
  case DMS_GET_IMS_SUBSCRIPTION_STATUS:
    logger(MSG_INFO, "%s: IMS Subscription status change indication\n",
           __func__);
    break;

  case DMS_GET_MODEL:
    check_and_set_fw_string(buf, buf_len, 0x01, dms_runtime.modem_model);
    break;
  case DMS_GET_REVISION:
    check_and_set_fw_string(buf, buf_len, 0x01, dms_runtime.modem_revision);
    break;
  case DMS_GET_IDS:
    check_and_set_fw_string(buf, buf_len, 0x12, dms_runtime.modem_serial_num);
    print_modem_information();
    break;
  case DMS_GET_HARDWARE_REVISION:
    check_and_set_fw_string(buf, buf_len, 0x01, dms_runtime.modem_hw_rev);
    print_modem_information();
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
    print_modem_information();
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for DMS: %.4x\n", __func__,
           get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}

void dms_retrieve_modem_info() {
  dms_request_model();
  dms_request_hw_rev();
  dms_request_revision();
  dms_request_serial_number();
  dms_request_sw_ver();
 return;
}