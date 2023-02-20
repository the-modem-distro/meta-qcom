// SPDX-License-Identifier: MIT

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/nas.h"
#include "../inc/qmi.h"
#include "../inc/sms.h"

// #define DEBUG_NAS 0

struct {
  /* Basic state */
  struct basic_network_status curr_state;
  struct basic_network_status prev_state;
  uint8_t curr_plmn[3];
  uint8_t prev_plmn[3];

  /* Network status report history */
  uint16_t current_report;
  struct network_status_reports data[MAX_REPORT_NUM];

  /* Latest retrieved Cell ID and LAC/TAC */
  uint32_t current_cell_id;
  uint16_t current_lac;

  /* Open Cellid data */
  uint8_t cellid_data_missing; // 0 missing, 1 ready, 2 missing and requested
  FILE *curr_ocid_db_fp;
  uint64_t open_cellid_num_items;
  uint8_t open_cellid_mcc[4];
  uint8_t open_cellid_mnc[3];

} nas_runtime;

/*
 * OpenCellid base functions
 */

struct ocid_cell_slim get_opencellid_cell_info(uint32_t cell_id, uint16_t lac) {
  struct ocid_cell_slim *ocid = malloc(sizeof(struct ocid_cell_slim));
  struct ocid_cell_slim cell = {0};

  if (nas_runtime.cellid_data_missing != 1) {
    logger(MSG_ERROR, "%s: Open Cellid data isn't available\n", __func__);
    free(ocid);
    return cell;
  }

  if (nas_runtime.curr_ocid_db_fp == NULL) {
    nas_runtime.cellid_data_missing = 0;
    logger(MSG_ERROR, "%s: File is missing!\n", __func__);
    free(ocid);
    return cell;
  }

  fseek(nas_runtime.curr_ocid_db_fp, 0L, SEEK_SET);

  logger(MSG_DEBUG, "%s Looking for the cell id\n", __func__);
  for (uint64_t i = 0; i < nas_runtime.open_cellid_num_items; i++) {
    memset(ocid, 0, sizeof(struct ocid_cell_slim));
    if (fread(ocid, sizeof(struct ocid_cell_slim), 1,
              nas_runtime.curr_ocid_db_fp) != 1) {
      logger(MSG_ERROR, "%s: Error reading OpenCellid database!\n", __func__);
      fclose(nas_runtime.curr_ocid_db_fp);
      nas_runtime.curr_ocid_db_fp = NULL;
      free(ocid);
      return cell;
    }
    if (nas_runtime.curr_state.network_type == ocid->radio &&
        cell_id == ocid->cell && ocid->area == lac) {
      logger(MSG_INFO, "%s: Found %.8x %.4x (OpenCellID: %.8x %.4x)\n", __func__,
             cell_id, lac, ocid->cell, ocid->area);
      cell = *ocid;
      free(ocid);
      return cell;
    }
  }

  logger(MSG_INFO, "%s: Couldn't find cid %u with lac %u \n", __func__, cell_id,
         lac);
  free(ocid);
  return cell;
}

int is_cell_id_in_db(uint32_t cell_id, uint16_t lac) {
  struct ocid_cell_slim *ocid = malloc(sizeof(struct ocid_cell_slim));
  int ret = 0;
  if (nas_runtime.cellid_data_missing != 1) {
    logger(MSG_ERROR, "%s: Open Cellid data isn't available\n", __func__);
    free(ocid);
    return -EINVAL;
  }

  *ocid = get_opencellid_cell_info(cell_id, lac);
  if (nas_runtime.curr_state.network_type == ocid->radio &&
      cell_id == ocid->cell && ocid->area == lac) {
    ret = 1;
  }

  free(ocid);
  return ret;
}

uint8_t is_cellid_data_missing() {
  if (nas_is_network_in_service())
    return nas_runtime.cellid_data_missing;
  return 2;
}

void set_cellid_data_missing_as_requested() {
  nas_runtime.cellid_data_missing = 2;
}

void get_opencellid_data() {
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  size_t strsz = 0;
  char filename[256];
  size_t filesize = 0;
  snprintf(filename, 255, "/tmp/%s-%s.bin", (char *)nas_runtime.curr_state.mcc,
           (char *)nas_runtime.curr_state.mnc);

  if (nas_runtime.curr_ocid_db_fp != NULL) {
    logger(MSG_WARN, "%s: It seems we changed carriers!\n", __func__);
    fclose(nas_runtime.curr_ocid_db_fp);
    nas_runtime.curr_ocid_db_fp = NULL;
    nas_runtime.open_cellid_num_items = 0;
  }

  nas_runtime.curr_ocid_db_fp = fopen(filename, "r");
  if (nas_runtime.curr_ocid_db_fp == NULL) {
    logger(
        MSG_WARN,
        "%s: Can't find OpenCellid database for the current carrier: %s-%s\n",
        __func__, nas_runtime.curr_state.mcc, nas_runtime.curr_state.mnc);
    if (nas_runtime.cellid_data_missing < 2) {
      set_cellid_data_missing_as_requested();
      notify_database_unavailable();
    }
    return;
  }

  /* I probably should add a header to each file in ocid_conv */
  fseek(nas_runtime.curr_ocid_db_fp, 0L, SEEK_END);
  filesize = ftell(nas_runtime.curr_ocid_db_fp);
  fseek(nas_runtime.curr_ocid_db_fp, 0L, SEEK_SET);
  nas_runtime.open_cellid_num_items = filesize / sizeof(struct ocid_cell_slim);
  memcpy(nas_runtime.open_cellid_mcc, nas_runtime.curr_state.mcc, 4);
  memcpy(nas_runtime.open_cellid_mnc, nas_runtime.curr_state.mnc, 3);
  nas_runtime.cellid_data_missing = 1;

  strsz = snprintf(
      (char *)reply, MAX_MESSAGE_SIZE, "OpenCellid database for %s-%s loaded",
      (char *)nas_runtime.curr_state.mcc, (char *)nas_runtime.curr_state.mnc);
  add_message_to_queue(reply, strsz);
  logger(MSG_INFO, "%s: End\n", __func__);
}

/*
 * Hardcoded user notifications
 */

void notify_database_unavailable() {
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  size_t strsz =
      snprintf((char *)reply, MAX_MESSAGE_SIZE,
               "OpenCellID database is not available. Please check "
               "https://opencellid.themodemdistro.com for more info");
  add_message_to_queue(reply, strsz);
}

void fallback_notify() {
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                          "Signal Tracking warning! I can't load any previous "
                          "cell data report, I'm switching to learning mode\n");
  add_message_to_queue(reply, strsz);
}

void emergency_baseband_pwoerdown(uint32_t cell_id, uint16_t lac,
                                  int opencellid_res) {
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};

  size_t strsz = snprintf(
      (char *)reply, MAX_MESSAGE_SIZE,
      "Emergency: I connected to an unknown Cell ID and will lock myself up\n"
      "Reboot me to use me again\n"
      "Cell ID: %.8x\nLAC: %.4x\n",
      cell_id, lac);
  if (opencellid_res == 1) {
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "OpenCellid DB check: Found in db");
  } else if (opencellid_res == 0) {
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "OpenCellid DB check: Not found in db!");
  } else {
    strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                      "OpenCellid DB check: Database not loaded!");
  }
  add_message_to_queue(reply, strsz);
  // Disable until we get to production with this
  /*  sleep(10);
    if (write_to(ADSP_BOOT_HANDLER, "0", O_WRONLY) < 0) {
      logger(MSG_ERROR, "%s: Error shutting down the ADSP\n", __func__);
    }*/
}

void notify_cellid_change(uint32_t cell_id, uint16_t lac) {
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                          "Network status changed:\n"
                          "Location Area / Cell ID changed:\n"
                          "Cell ID: %.8x\nLAC: %.4x\n",
                          cell_id, lac);
  if (get_signal_tracking_mode() > 1) {
    struct ocid_cell_slim cell = get_opencellid_cell_info(cell_id, lac);
    if (cell.cell != 0 && cell.area != 0) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "OpenCellid Verified:"
                        "Cell ID: %.8x\nLAC: %.4x\n"
                        "Lat: %f\nLon: %f\n",
                        cell.cell, cell.area, cell.lat, cell.lon);
    } else {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Not found in OpenCellid");
    }
  }
  add_message_to_queue(reply, strsz);
}

/*
  Save & retrieve previous reports
*/
int store_report_data() {
  FILE *fp;
  int ret;
  logger(MSG_DEBUG, "%s: Start\n", __func__);
  if (set_persistent_partition_rw() < 0) {
    logger(MSG_ERROR, "%s: Can't set persist partition in RW mode\n", __func__);
    return -1;
  }
  logger(MSG_DEBUG, "%s: Open file\n", __func__);
  fp = fopen(INTERNAL_CELLID_INFO_PATH, "w");
  if (fp == NULL) {
    logger(MSG_ERROR, "%s: Can't open config file for writing\n", __func__);
    return -1;
  }
  logger(MSG_DEBUG, "%s: Store\n", __func__);
  ret = fwrite(nas_runtime.data, sizeof(struct network_status_reports),
               MAX_REPORT_NUM, fp);
  logger(MSG_DEBUG, "%s: Close (%i bytes written)\n", __func__, ret);
  fclose(fp);
  do_sync_fs();
  if (!use_persistent_logging()) {
    if (set_persistent_partition_ro() < 0) {
      logger(MSG_ERROR, "%s: Can't set persist partition in RO mode\n",
             __func__);
      return -1;
    }
  }
  return 0;
}

int load_report_data() {
  FILE *fp;
  int ret;
  struct network_status_reports reports[MAX_REPORT_NUM];
  logger(MSG_DEBUG, "%s: Start, open file\n", __func__);
  fp = fopen(INTERNAL_CELLID_INFO_PATH, "r");
  if (fp == NULL) {
    logger(MSG_DEBUG, "%s: Can't open config file for reading\n", __func__);
    if (get_signal_tracking_mode() == 1 || get_signal_tracking_mode() == 3) {
      logger(MSG_ERROR,
             "%s: Error: There's no cell id data to use, mode falling back to "
             "learning mode\n",
             __func__);
      fallback_notify();
      if (get_signal_tracking_mode() == 1) {
        set_signal_tracking_mode(0);
      } else if (get_signal_tracking_mode() == 3) {
        set_signal_tracking_mode(2);
      }
    }
    return -1;
  }
  ret =
      fread(reports, sizeof(struct network_status_reports), MAX_REPORT_NUM, fp);
  if (ret >= sizeof(struct network_status_reports)) {
    logger(MSG_DEBUG, "%s: Loading previous reports...\n ", __func__);
    for (int i = 0; i < MAX_REPORT_NUM; i++) {
      if (reports[i].in_use) {
        nas_runtime.data[i] = reports[i];
        nas_runtime.current_report++;
      }
    }
  }
  logger(MSG_INFO, "%s finished, %i reports in memory\n", __func__,
         nas_runtime.current_report);
  fclose(fp);
  return 0;
}

/*
 * Some getters
 */

uint8_t *get_current_mcc() { return nas_runtime.curr_state.mcc; }
uint8_t *get_current_mnc() { return nas_runtime.curr_state.mnc; }
uint8_t get_network_type() { return nas_runtime.curr_state.network_type; }

struct basic_network_status get_network_status() {
  return nas_runtime.curr_state;
}

/* Used in command.c */
struct nas_report get_current_cell_report() {
  return nas_runtime.data[nas_runtime.current_report].report;
}

uint8_t get_signal_strength() { return nas_runtime.curr_state.signal_level; }

uint8_t nas_is_network_in_service() {
  if (nas_runtime.curr_state.service_capability.gprs ||
      nas_runtime.curr_state.service_capability.edge ||
      nas_runtime.curr_state.service_capability.hsdpa ||
      nas_runtime.curr_state.service_capability.hsupa ||
      nas_runtime.curr_state.service_capability.wcdma ||
      nas_runtime.curr_state.service_capability.gsm ||
      nas_runtime.curr_state.service_capability.lte ||
      nas_runtime.curr_state.service_capability.hsdpa_plus ||
      nas_runtime.curr_state.service_capability.dc_hsdpa_plus)
    return 1;

  return 0;
}

uint8_t has_capability_changed() {
  if (memcmp(&nas_runtime.curr_state.service_capability,
             &nas_runtime.prev_state.service_capability,
             sizeof(struct service_capability)) == 0)
    return 0;

  return 1;
}

const char *get_nas_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(nas_svc_commands) / sizeof(nas_svc_commands[0])); i++) {
    if (nas_svc_commands[i].id == msgid) {
      return nas_svc_commands[i].cmd;
    }
  }
  return "NAS: Unknown command\n";
}

int nas_register_to_events() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (33 * sizeof(struct qmi_generic_uint8_t_tlv));
  uint8_t *pkt = malloc(pkt_len);
  uint8_t tlvs[] = {
      NAS_SVC_INDICATION_SYS_SELECT,
      NAS_SVC_INDICATION_DDTM_EVENT,
      NAS_SVC_INDICATION_SERVING_SYS,
      NAS_SVC_INDICATION_DS_PREF,
      NAS_SVC_INDICATION_SUBSCRIPTION_INFO,
      NAS_SVC_INDICATION_NETWORK_TIME,
      NAS_SVC_INDICATION_SYS_INFO,
      NAS_SVC_INDICATION_SIGNAL_INFO,
      NAS_SVC_INDICATION_ERROR_RATE,
      NAS_SVC_INDICATION_MANAGED_ROAMING,
      NAS_SVC_INDICATION_CURRENT_PLMN,
      NAS_SVC_INDICATION_EMBMS_STATE,
      NAS_SVC_INDICATION_RF_BAND_INFO,
      NAS_SVC_INDICATION_NETWORK_REJECT_DATA,
      NAS_SVC_INDICATION_OPERATOR_NAME,
      NAS_SVC_INDICATION_PLMN_MODE_BIT,
      NAS_SVC_INDICATION_RTRE_CONFIG,
      NAS_SVC_INDICATION_IMS_PREFERENCE,
      NAS_SVC_INDICATION_EMERGENCY_STATE_READY,
      NAS_SVC_INDICATION_LTE_NETWORK_TIME,
      NAS_SVC_INDICATION_LTE_CARRIER_AGG_INFO,
      NAS_SVC_INDICATION_SUBSCRIPTION_CHANGE,
      NAS_SVC_INDICATION_SERVICE_ACCESS_CLASS_BARRING,
      NAS_SVC_INDICATION_DATA_SUBSCRIPTION_PRIORITY_CHANGE,
      NAS_SVC_INDICATION_CALL_MODE_STATUS,
      NAS_SVC_INDICATION_EMERGENCY_MODE_STATUS,
      NAS_SVC_INDICATION_GET_CELL_INFO,
      NAS_SVC_INDICATION_EXTENDED_DISCONTINUOUS_RECEIVE_CHANGE,
      NAS_SVC_INDICATION_LTE_RACH_FAILURE_INFO,
      NAS_SVC_INDICATION_LTE_RRC_TX_INFO,
      NAS_SVC_INDICATION_SUB_BLOCK_STATUS_INFO,
      NAS_SVC_INDICATION_EMERGENCY_911_SEARCH_FAILURE_INFO,
      NAS_SVC_INDICATION_ARFCN_LIST_INFO,
      NAS_SVC_INDICATION_GET_RF_AVAILABILITY,
  };
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_NAS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, NAS_REGISTER_INDICATIONS) <
      0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  for (int i = 0; i < 19; i++) {
    if (build_u8_tlv(pkt, pkt_len, curr_offset, tlvs[i], 1) < 0) {
      logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  }

  add_pending_message(QMI_SERVICE_NAS, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}

int nas_request_cell_location_info() { // QMI_NAS_GET_SIG_INFO
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_NAS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       NAS_GET_CELL_LOCATION_INFO) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_NAS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int nas_request_signal_info() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_NAS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, NAS_GET_SIGNAL_INFO) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_NAS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int nas_get_ims_preference() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_NAS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       NAS_GET_IMS_PREFERENCE_STATE) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_NAS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

void update_operator_name(uint8_t *buf, size_t buf_len) {
  /* TLVS here: 0x10, 0x11, 0x12 (we discard this one since it's the same as
   * 0x10 but encoded in gsm7)*/
  int offset = get_tlv_offset_by_id(buf, buf_len, 0x10);
  if (offset > 0) {
    struct carrier_name_string *name =
        (struct carrier_name_string *)(buf + offset);
    if (buf_len < name->len) {
      logger(MSG_ERROR, "%s: Message is shorter than the operator name\n",
             __func__);
      return;
    }
    memcpy(nas_runtime.curr_state.operator_name, name->operator_name,
           name->len - (sizeof(uint16_t)));
  }
  offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
  if (offset > 0) {
    struct carrier_mcc_mnc *op_code = (struct carrier_mcc_mnc *)(buf + offset);
    if (buf_len < op_code->len) {
      logger(MSG_ERROR, "%s: Message is shorter than carrier data\n", __func__);
      return;
    }
    memcpy(nas_runtime.curr_state.mcc, op_code->mcc, 3);
    memcpy(nas_runtime.curr_state.mnc, op_code->mnc, 2);
    nas_runtime.curr_state.location_area_code_1 = op_code->lac1;
    nas_runtime.curr_state.location_area_code_2 = op_code->lac2;
  }

  logger(MSG_INFO,
         "%s: Operator: %s | "
         "MCC-MNC: %s-%s | "
         "LACs: %.4x %.4x\n",
         __func__, nas_runtime.curr_state.operator_name,
         (unsigned char *)nas_runtime.curr_state.mcc,
         (unsigned char *)nas_runtime.curr_state.mnc,
         nas_runtime.curr_state.location_area_code_1,
         nas_runtime.curr_state.location_area_code_2);
}

void parse_serving_system_message(uint8_t *buf, size_t buf_len) {
  /* There can be a lot of tlvs here, but we care about 2, service and
   * capability*/
  int offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
  if (offset > 0) {
    struct empty_tlv *capability_arr = (struct empty_tlv *)(buf + offset);
    if (buf_len < capability_arr->len) {
      logger(MSG_ERROR, "%s: Message is shorter than the operator name\n",
             __func__);
      return;
    }
    nas_runtime.prev_state = nas_runtime.curr_state;

    // Set everything to 0, then repopulate
    nas_runtime.curr_state.service_capability.gprs = 0;
    nas_runtime.curr_state.service_capability.edge = 0;
    nas_runtime.curr_state.service_capability.hsdpa = 0;
    nas_runtime.curr_state.service_capability.hsupa = 0;
    nas_runtime.curr_state.service_capability.wcdma = 0;
    nas_runtime.curr_state.service_capability.gsm = 0;
    nas_runtime.curr_state.service_capability.lte = 0;
    nas_runtime.curr_state.service_capability.hsdpa_plus = 0;
    nas_runtime.curr_state.service_capability.dc_hsdpa_plus = 0;

    for (uint16_t i = 0; i < capability_arr->len; i++) {
      switch (capability_arr->data[i]) {
      case 0x00: // When is not yet connected to the network
        logger(MSG_INFO, "%s: No service\n", __func__);
        break;
      case 0x01:
        nas_runtime.curr_state.service_capability.gprs = 1;
        break;
      case 0x02:
        nas_runtime.curr_state.service_capability.edge = 1;
        break;
      case 0x03:
        nas_runtime.curr_state.service_capability.hsdpa = 1;
        break;
      case 0x04:
        nas_runtime.curr_state.service_capability.hsupa = 1;
        break;
      case 0x05:
        nas_runtime.curr_state.service_capability.wcdma = 1;
        break;
      case 0x0a:
        nas_runtime.curr_state.service_capability.gsm = 1;
        break;
      case 0x0b:
        nas_runtime.curr_state.service_capability.lte = 1;
        break;
      case 0x0c:
        nas_runtime.curr_state.service_capability.hsdpa_plus = 1;
        break;
      case 0x0d:
        nas_runtime.curr_state.service_capability.dc_hsdpa_plus = 1;
        break;
      default:
        logger(MSG_WARN, "%s: Unknown service capability: %.2x\n", __func__,
               capability_arr->data[i]);
        break;
      }
    }
  }
  offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
  if (offset > 0) {
    struct nas_serving_system_state *serving_sys =
        (struct nas_serving_system_state *)(buf + offset);
    if (buf_len < serving_sys->len) {
      logger(MSG_ERROR, "%s: Message is shorter than carrier data\n", __func__);
      return;
    }
    nas_runtime.curr_state.network_registration_status =
        serving_sys->registration_status;
    nas_runtime.curr_state.circuit_switch_attached = serving_sys->cs_attached;
    nas_runtime.curr_state.packet_switch_attached = serving_sys->ps_attached;
    nas_runtime.curr_state.radio_access = serving_sys->radio_access;
  }

  logger(MSG_DEBUG,
         "%s: Capabilities\n"
         "\tGSM: %s"
         "\tGPRS: %s"
         "\tEDGE: %s\n"
         "\tWCDMA: %s"
         "\tHSDPA: %s"
         "\tHSUPA: %s\n"
         "\tHSDPA+: %s"
         "\tDC-HSDPA+: %s"
         "\tLTE: %s\n"
         "\tCircuit Switch Service Attached: %s\n"
         "\tPacket Switch Service attached: %s\n",
         __func__,
         nas_runtime.curr_state.service_capability.gsm == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.gprs == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.edge == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.wcdma == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.hsdpa == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.hsupa == 1 ? "Yes" : "No",
         nas_runtime.curr_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                   : "No",
         nas_runtime.curr_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                      : "No",
         nas_runtime.curr_state.service_capability.lte == 1 ? "Yes" : "No",
         nas_runtime.curr_state.circuit_switch_attached == 1 ? "Yes" : "No",
         nas_runtime.curr_state.packet_switch_attached == 1 ? "Yes" : "No");

  if (nas_runtime.prev_state.packet_switch_attached &&
      !nas_runtime.prev_state.packet_switch_attached &&
      is_signal_tracking_enabled()) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "Warning: Packet switch detached!");
    add_message_to_queue(reply, strsz);
  }
  if (nas_runtime.prev_state.circuit_switch_attached &&
      !nas_runtime.prev_state.circuit_switch_attached) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "Warning: Circuit switch detached!");
    add_message_to_queue(reply, strsz);
  }
  if (is_signal_tracking_enabled() && has_capability_changed()) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Change in service cap.\n"
        "GSM:%s\n"
        "GPRS:%s\n"
        "EDGE:%s\n"
        "WCDMA:%s\n"
        "HSDPA:%s\n"
        "HSUPA:%s\n"
        "HSDPA+:%s\n"
        "DC-HSDPA+:%s\n"
        "LTE:%s\n"
        "CS Attached:%s\n"
        "PS Attached:%s\n",
        nas_runtime.curr_state.service_capability.gsm == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.gprs == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.edge == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.wcdma == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.hsdpa == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.hsupa == 1 ? "Yes" : "No",
        nas_runtime.curr_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                  : "No",
        nas_runtime.curr_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                     : "No",
        nas_runtime.curr_state.service_capability.lte == 1 ? "Yes" : "No",
        nas_runtime.curr_state.circuit_switch_attached == 1 ? "Yes" : "No",
        nas_runtime.curr_state.packet_switch_attached == 1 ? "Yes" : "No");
    add_message_to_queue(reply, strsz);
  }
}
/*
 *  Operating Modes:
 *    0 -> Retrieved network info only: learn and notify
 *    1 -> Retrieved network info only: strict (don't learn, shutdown
 * automatically) 2 -> Retrieved network info + opencellid: learn and notify 3
 * -> Retrieved network info + opencellid: strict (don't learn, shutdown
 * automatically)
 *
 *
 *  Pending tasks:
 *    X Save & Retrieve report dumpfile
 *    X Automatically back up from mode 1 to 0 if report doesn't exist
 *    * Stop nagging all the time. Once is enough (notification spam on unknown
 * cells) (is it working fine?)
 *    * Try to gather neighbour cell ids: Can't find how so far
 *    WIP Location? In reports from OCID. Investigate PDSv2 later on
 */

int add_report(uint16_t mcc, uint16_t mnc, uint8_t type_of_service,
               uint16_t lac, uint16_t phy_cell_id, uint32_t cell_id,
               uint8_t bsic, uint16_t bcch, uint16_t psc, uint16_t arfcn,
               int16_t srx_lev, uint16_t rx_lev) {
  int report_id = 0;
  if (nas_runtime.current_report >= (MAX_REPORT_NUM - 1)) {
    logger(MSG_INFO, "%s: Rotating log...\n", __func__);
    for (int k = 1; k < MAX_REPORT_NUM; k++) {
      nas_runtime.data[k - 1] = nas_runtime.data[k];
    }
  } else {
    nas_runtime.current_report++;
  }
  logger(MSG_INFO, "%s: Report ID %i\n", __func__, nas_runtime.current_report);
  report_id = nas_runtime.current_report;
  nas_runtime.data[report_id].in_use = 1;
  nas_runtime.data[report_id].report.found_in_network = 1;
  nas_runtime.data[report_id].report.mcc = mcc;
  nas_runtime.data[report_id].report.mnc = mnc;
  nas_runtime.data[report_id].report.type_of_service = type_of_service;
  nas_runtime.data[report_id].report.lac = lac;
  nas_runtime.data[report_id].report.phy_cell_id = phy_cell_id;
  nas_runtime.data[report_id].report.cell_id = cell_id;
  nas_runtime.data[report_id].report.bsic = bsic;
  nas_runtime.data[report_id].report.bcch = bcch;
  nas_runtime.data[report_id].report.psc = psc;
  nas_runtime.data[report_id].report.arfcn = arfcn;
  nas_runtime.data[report_id].report.srx_level_min = srx_lev;
  nas_runtime.data[report_id].report.srx_level_max = srx_lev;
  nas_runtime.data[report_id].report.rx_level_min = rx_lev;
  nas_runtime.data[report_id].report.rx_level_max = rx_lev;
  nas_runtime.data[report_id].report.opencellid_verified = 0;

  return report_id;
}
int is_in_report(uint16_t mcc, uint16_t mnc, uint8_t type_of_service,
                 uint32_t cell_id, uint16_t lac) {
  int report_id = -1;
  for (int i = 0; i < nas_runtime.current_report; i++) {
    if (nas_runtime.data[i].report.mcc == mcc &&
        nas_runtime.data[i].report.mnc == mnc &&
        nas_runtime.data[i].report.type_of_service == type_of_service &&
        nas_runtime.data[i].report.cell_id == cell_id &&
        nas_runtime.data[i].report.lac == lac) {
      logger(MSG_INFO, "%s: Cell found in report %i\n", __func__, i);
      nas_runtime.data[i].in_use = 1;
      nas_runtime.data[i].report.found_in_network = 1;
      report_id = i;
    }
  }
  if (report_id < 0) {
    logger(MSG_WARN, "%s: Cell not found!\n", __func__);
  }

  store_report_data();
  return report_id;
}

void process_current_network_data(uint16_t mcc, uint16_t mnc,
                                  uint8_t type_of_service, uint16_t lac,
                                  uint16_t phy_cell_id, uint32_t cell_id,
                                  uint8_t bsic, uint16_t bcch, uint16_t psc,
                                  uint16_t arfcn, int16_t srx_lev,
                                  uint16_t rx_lev) {
  uint8_t cell_is_known = 0;
  int report_id = -1;
  int opencellid_res = -1;

  if (lac == 0 || cell_id == 0) {
    logger(MSG_WARN, "%s: Not enough data to process\n", __func__);
    return;
  }
  report_id = is_in_report(mcc, mnc, type_of_service, cell_id, lac);

  /* Update signal levels, bcch etc.*/
  if (report_id >= 0) {
    cell_is_known = 1;
    if (srx_lev < nas_runtime.data[report_id].report.srx_level_min) {
      nas_runtime.data[report_id].report.srx_level_min = srx_lev;
    } else if (srx_lev > nas_runtime.data[report_id].report.srx_level_max) {
      nas_runtime.data[report_id].report.srx_level_max = srx_lev;
    }
    nas_runtime.data[report_id].report.phy_cell_id = phy_cell_id;
    nas_runtime.data[report_id].report.bsic = bsic;
    nas_runtime.data[report_id].report.bcch = bcch;
    nas_runtime.data[report_id].report.psc = psc;
    nas_runtime.data[report_id].report.arfcn = arfcn;
    store_report_data();
  }

  switch (get_signal_tracking_mode()) {
  case 0:
    logger(MSG_INFO, "%s: Learning mode: standalone\n", __func__);
    if (!cell_is_known) {
      report_id = add_report(mcc, mnc, type_of_service, lac, phy_cell_id,
                             cell_id, bsic, bcch, psc, arfcn, srx_lev, rx_lev);
    }
    break;
  case 1:
    logger(MSG_INFO, "%s: Strict mode: standalone\n", __func__);
    if (!cell_is_known) {
      emergency_baseband_pwoerdown(cell_id, lac, opencellid_res);
    }
    break;
  case 2:
    logger(MSG_INFO, "%s: Learning mode: OpenCellid + standalone\n", __func__);
    opencellid_res = is_cell_id_in_db(cell_id, lac);

    if (!cell_is_known) {
      report_id = add_report(mcc, mnc, type_of_service, lac, phy_cell_id,
                             cell_id, bsic, bcch, psc, arfcn, srx_lev, rx_lev);
      if (opencellid_res == 1) {
        nas_runtime.data[report_id].report.opencellid_verified = 1;
      }
    }
    break;
  case 3:
    opencellid_res = is_cell_id_in_db(cell_id, lac);
    if (opencellid_res == 1 && !cell_is_known) {
      report_id = add_report(mcc, mnc, type_of_service, lac, phy_cell_id,
                             cell_id, bsic, bcch, psc, arfcn, srx_lev, rx_lev);
      nas_runtime.data[report_id].report.opencellid_verified = 1;
    } else if (!cell_is_known) {
      emergency_baseband_pwoerdown(cell_id, lac, opencellid_res);
    } else {
      logger(
          MSG_WARN,
          "%s: Don't know what to do! Cell is known: %u | opencellid_res: %i\n",
          __func__, cell_is_known, opencellid_res);
    }
    logger(MSG_INFO, "%s: Strict mode: OpenCellid + standalone\n", __func__);
    break;

  default:
    logger(MSG_ERROR,
           "%s: Fatal: Unknown signal tracking mode set up in config\n",
           __func__);
    break;
  }

  if (cell_id != nas_runtime.current_cell_id ||
      lac != nas_runtime.current_lac) {
    notify_cellid_change(cell_id, lac);
    nas_runtime.current_cell_id = cell_id;
    nas_runtime.current_lac = lac;
  }
}

void update_cell_location_information(uint8_t *buf, size_t buf_len) {
  uint16_t mcc = 0, mnc = 0, lac = 0, phy_cell_id = 0, bcch = 0, psc = 0,
           arfcn = 0, srx_lev = 0, rx_lev = 0;
  uint8_t type_of_service = 0, bsic = 0;
  uint32_t cell_id = 0;
  uint8_t reported_plmn[3] = { 0 };
  if (!is_signal_tracking_enabled()) {
    logger(MSG_INFO, "%s: Tracking is disabled\n", __func__);
    return;
  }

  if (did_qmi_op_fail(buf, buf_len) != QMI_RESULT_SUCCESS) {
    logger(MSG_ERROR, "%s: Baseband returned an error to the request \n",
           __func__);
    return;
  }

  if (get_signal_tracking_mode() > 1) {
    if (memcmp(nas_runtime.curr_state.mcc, nas_runtime.open_cellid_mcc, 4) !=
            0 ||
        memcmp(nas_runtime.curr_state.mnc, nas_runtime.open_cellid_mnc, 3) !=
            0) {
      logger(MSG_INFO, "%s: Needs OpenCellid data refresh!\n", __func__);
      get_opencellid_data();
    }
  }

  mcc = atoi((char *)nas_runtime.curr_state.mcc);
  mnc = atoi((char *)nas_runtime.curr_state.mnc);

  uint8_t available_tlvs[] = {
      NAS_CELL_LAC_INFO_UMTS_CELL_INFO,
      NAS_CELL_LAC_INFO_CDMA_CELL_INFO,
      NAS_CELL_LAC_INFO_LTE_INTRA_INFO,
      NAS_CELL_LAC_INFO_LTE_INTER_INFO,
      NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM,
      NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA,
      NAS_CELL_LAC_INFO_UMTS_CELL_ID,
      NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR,
      NAS_CELL_LAC_INFO_CDMA_RX_INFO,
      NAS_CELL_LAC_INFO_HDR_RX_INFO,
      NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED,
      NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED,
      NAS_CELL_LAC_INFO_WCDMA_GSM_NEIGHBOUT_CELL_EXTENDED,
      NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV,
      NAS_CELL_LAC_INFO_WCDMA_INFO_ACTIVE_SET,
      NAS_CELL_LAC_INFO_WCDMA_ACTIVE_SET_REF_RADIO_LINK,
      NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO,
      NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO,
      NAS_CELL_LAC_INFO_WCDMA_EXTENDED_INFO_AS,
      NAS_CELL_LAC_INFO_SCELL_GERAN_CONF,
      NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT,
      NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ,
      NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN,
      NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN,
      NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN,
      NAS_CELL_LAC_INFO_NAS_INFO_EMM_STATE,
      NAS_CELL_LAC_INFO_NAS_RRC_STATE,
      NAS_CELL_LAC_INFO_LTE_INFO_RRC_STATE,
  };

  logger(MSG_DEBUG, "%s: Found %u information segments in this message\n",
         __func__, count_tlvs_in_message(buf, buf_len));
  for (uint8_t i = 0; i < 27; i++) {
    int offset = get_tlv_offset_by_id(buf, buf_len, available_tlvs[i]);
    if (offset > 0) {
      logger(MSG_DEBUG, "%s: TLV %.2x found at offset %.2x\n", __func__,
             available_tlvs[i], offset);
      switch (available_tlvs[i]) {
      case NAS_CELL_LAC_INFO_UMTS_CELL_INFO: {
        struct nas_lac_umts_cell_info *cell_info =
            (struct nas_lac_umts_cell_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_UMTS_CELL_INFO\n"
               "Cell ID: %.4x\n"
               "\t Location Area Code: %.4x\n"
               "\t UARFCN: %.4x\n"
               "\t PSC: %.4x\n"
               "\t RSCP: %i\n"
               "\t Signal level: %i\n"
               "\t ECIO: %i\n"
               "\t Monitored cell count: %u\n",
               __func__, cell_info->cell_id, cell_info->lac, cell_info->uarfcn,
               cell_info->psc, cell_info->rscp, cell_info->ecio,
               cell_info->instances);

        type_of_service = OCID_RADIO_UMTS;
        cell_id = cell_info->cell_id;
        lac = cell_info->lac;
        srx_lev = cell_info->rscp;
        arfcn = cell_info->uarfcn;
        psc = cell_info->psc;
        memcpy(reported_plmn, cell_info->plmn, 3);
        for (uint8_t j = 0; j < cell_info->instances; j++) {
          logger(MSG_DEBUG,
                 "Neighbour %u\n"
                 "\t\t UARFCN: %.4x\n"
                 "\t\t PSC: %.4x\n"
                 "\t\t RSCP: %i\n"
                 "\t\t ECIO: %i\n",
                 j, cell_info->monitored_cells[j].uarfcn,
                 cell_info->monitored_cells[j].psc,
                 cell_info->monitored_cells[j].rscp,
                 cell_info->monitored_cells[j].ecio);
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INTRA_INFO: {
        struct nas_lac_lte_intra_cell_info *cell_info =
            (struct nas_lac_lte_intra_cell_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INTRA_INFO\n"
               "\t Is Idle?: %s\n"
               "\t Cell ID: %.4x\n"
               "\t Tracking Area Code: %.4x\n"
               "\t EARFCN: %.4x\n"
               "\t Serving Cell ID: %.4x\n"
               "\t Cell Reselect Priority: %.2x\n"
               "\t Non Intrafrequency search threshold: %.2x\n"
               "\t Serving cell lower threshold: %.2x\n"
               "\t Intrafrequency search threshold: %.2x\n"
               "\t Neighbour Cell count: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->cell_id, cell_info->tracking_area_code,
               cell_info->earfcn, cell_info->serv_cell_id,
               cell_info->cell_reselect_prio,
               cell_info->non_intra_search_threshold,
               cell_info->serving_cell_lower_threshold,
               cell_info->intra_search_threshold, 
               cell_info->num_of_cells);

        type_of_service = OCID_RADIO_LTE;
        cell_id = cell_info->cell_id;
        lac = cell_info->tracking_area_code;
        arfcn = cell_info->earfcn;
        // cell_info->serv_cell_id; // FIXME: We seem to have cell id in u16 and
        // u32 values...
        memcpy(reported_plmn, cell_info->plmn, 3);

        for (uint8_t j = 0; j < cell_info->num_of_cells; j++) {
          logger(MSG_DEBUG,
                 "Neighbour %u\n"
                 "\t\t PHY Cell ID: %.4x\n"
                 "\t\t RSRQ: %.4x\n"
                 "\t\t RSRP: %i\n"
                 "\t\t RSSI Lev: %i\n"
                 "\t\t SRX Lev: %i\n",
                 j, cell_info->lte_cell_info[j].phy_cell_id,
                 cell_info->lte_cell_info[j].rsrq,
                 cell_info->lte_cell_info[j].rsrp,
                 cell_info->lte_cell_info[j].srx_level,
                 cell_info->lte_cell_info[j].rssi_level);
          // add neighbours too?
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INTER_INFO:

      {
        struct nas_lac_lte_inter_cell_info *cell_info =
            (struct nas_lac_lte_inter_cell_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INTER_INFO\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_DEBUG,
                 "Instance %u\n"
                 "\t\t EARFCN: %.4x\n"
                 "\t\t SRX Level Low Threshold: %.2x\n"
                 "\t\t SRX Level High Threshold: %.2x\n"
                 "\t\t Reselect Priority: %.2x\n"
                 "\t\t Number of cells: %i\n",
                 j, cell_info->lte_inter_freq_instance[j].earfcn,
                 cell_info->lte_inter_freq_instance[j].srxlev_low_threshold,
                 cell_info->lte_inter_freq_instance[j].srxlev_high_threshold,
                 cell_info->lte_inter_freq_instance[j].reselect_priority,
                 cell_info->lte_inter_freq_instance[j].num_cells);
          for (uint8_t k = 0;
               k < cell_info->lte_inter_freq_instance[j].num_cells; k++) {
            logger(MSG_DEBUG,
                   "\t Cell count %u\n"
                   "\t\t PHY Cell ID: %.4x\n"
                   "\t\t RSRQ: %.4x\n"
                   "\t\t RSRP: %i\n"
                   "\t\t RSSI Lev: %i\n"
                   "\t\t SRX Lev: %i\n",
                   k,
                   cell_info->lte_inter_freq_instance[j]
                       .lte_cell_info[k]
                       .phy_cell_id,
                   cell_info->lte_inter_freq_instance[j].lte_cell_info[k].rsrq,
                   cell_info->lte_inter_freq_instance[j].lte_cell_info[k].rsrp,
                   cell_info->lte_inter_freq_instance[j]
                       .lte_cell_info[k]
                       .srx_level,
                   cell_info->lte_inter_freq_instance[j]
                       .lte_cell_info[k]
                       .rssi_level);
          }
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM: {
        struct nas_lac_lte_gsm_neighbour_info *cell_info =
            (struct nas_lac_lte_gsm_neighbour_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(
              MSG_DEBUG,
              "\t Instance %u\n"
              "\t\t Cell Reselect Priority: %.2x\n"
              "\t\t Reselect threshold (hi): %.2x\n"
              "\t\t Reselect threshold (low): %.2x\n"
              "\t\t NCC mask: %.2x\n"
              "\t\t Number of cells: %i\n",
              j, cell_info->lte_gsm_neighbours[j].cell_reselect_priority,
              cell_info->lte_gsm_neighbours[j].high_priority_reselect_threshold,
              cell_info->lte_gsm_neighbours[j].low_prio_reselect_threshold,
              cell_info->lte_gsm_neighbours[j].ncc_mask,
              cell_info->lte_gsm_neighbours[j].num_cells);
          for (uint8_t k = 0; k < cell_info->lte_gsm_neighbours[j].num_cells;
               k++) {
            logger(
                MSG_DEBUG,
                "Cell count %u\n"
                "\t\t ARFCN: %.4x\n"
                "\t\t Band: %.2x\n"
                "\t\t Is cell ID valid? %.2x\n"
                "\t\t BSIC: %.2x\n"
                "\t\t RSSI: %i\n"
                "\t\t SRX Lev: %i\n",
                k,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .arfcn,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].band,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .is_cell_id_valid,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].bsic,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].rssi,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .srxlev

            );
          }
        }

      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA: {
        struct nas_lac_lte_wcdma_neighbour_info *cell_info =
            (struct nas_lac_lte_wcdma_neighbour_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_DEBUG,
                 "Instance %u\n"
                 "\t\t Cell Reselect Priority: %.2x\n"
                 "\t\t Reselect threshold (hi): %.4x\n"
                 "\t\t Reselect threshold (low): %.4x\n"
                 "\t\t UARFCN %.4x\n"
                 "\t\t Number of cells: %i\n",
                 j, cell_info->lte_wcdma_neighbours[j].cell_reselect_priority,
                 cell_info->lte_wcdma_neighbours[j]
                     .high_priority_reselect_threshold,
                 cell_info->lte_wcdma_neighbours[j].low_prio_reselect_threshold,
                 cell_info->lte_wcdma_neighbours[j].uarfcn,
                 cell_info->lte_wcdma_neighbours[j].num_cells);
          for (uint8_t k = 0; k < cell_info->lte_wcdma_neighbours[j].num_cells;
               k++) {
            logger(MSG_DEBUG,
                   "\t Cell count %u\n"
                   "\t\t PSC: %.4x\n"
                   "\t\t CPICH_RSCP: %.2x\n"
                   "\t\t CPICH_ECNO %.2x\n"
                   "\t\t SRX Level: %.2x\n",
                   k, cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].psc,
                   cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].cpich_rscp,
                   cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].cpich_ecno,
                   cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].srx_level);
          }
        }

      }

      break;
      case NAS_CELL_LAC_INFO_UMTS_CELL_ID: {
        struct nas_lac_umts_cell_id *cell_info =
            (struct nas_lac_umts_cell_id *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: Cell ID\n"
               "\t Type: UMTS\n"
               "\t Cell ID: %.4x\n",
               __func__, cell_info->cell_id);
        cell_id = cell_info->cell_id;
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR: {
        struct nas_lac_wcdma_lte_neighbour_info *cell_info =
            (struct nas_lac_wcdma_lte_neighbour_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR\n"
               "\t RRC State: %.4x\n"
               "\t Number of cells: %u\n",
               __func__, cell_info->rrc_state, cell_info->num_cells);

        for (uint8_t j = 0; j < cell_info->num_cells; j++) {
          logger(MSG_DEBUG,
                 "\t Cell %u\n"
                 "\t\t EARFCN %.4x\n"
                 "\t\t Physical Cell ID: %.4x\n"
                 "\t\t RSRP: %.4x\n"
                 "\t\t RSRQ: %.4x\n"
                 "\t\t SRX Level %i\n"
                 "\t\t Is cell TDD? %i\n",
                 j, cell_info->wcdma_lte_neighbour[j].earfcn,
                 cell_info->wcdma_lte_neighbour[j].phy_cell_id,
                 cell_info->wcdma_lte_neighbour[j].rsrp,
                 cell_info->wcdma_lte_neighbour[j].rsrq,
                 cell_info->wcdma_lte_neighbour[j].srx_level,
                 cell_info->wcdma_lte_neighbour[j].is_cell_tdd);
        }
      } break;
      case NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED: {
        struct nas_lac_extended_gsm_cell_info *cell_info =
            (struct nas_lac_extended_gsm_cell_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED\n"
               "\t Timing Advance: %.4x\n"
               "\t Channel Freq.: %.4x\n",
               __func__, cell_info->timing_advance, cell_info->bcch);
        bcch = cell_info->bcch;
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED: {
        struct nas_lac_extended_wcdma_cell_info *cell_info =
            (struct nas_lac_extended_wcdma_cell_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED\n"
               "\t WCDMA Power (dB): %.4x\n"
               "\t WCDMA TX Power (dB): %.4x\n"
               "\t Downlink Error rate: %i %%\n",
               __func__, cell_info->wcdma_power_db,
               cell_info->wcdma_tx_power_db, cell_info->downlink_error_rate);

      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV: {
        struct nas_lac_lte_timing_advance_info *cell_info =
            (struct nas_lac_lte_timing_advance_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV\n"
               "\t Timing advance: %i\n",
               __func__, cell_info->timing_advance);
      } break;
      case NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO: {
        struct nas_lac_extended_geran_info *cell_info =
            (struct nas_lac_extended_geran_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO\n"
               "\t Cell ID: %.8x\n"
               "\t LAC: %.4x\n"
               "\t ARFCN: %.4x\n"
               "\t BSIC: %.2x\n"
               "\t Timing advance: %i\n"
               "\t RX Level: %.4x\n"
               "\t Number of instances: %u\n",
               __func__, cell_info->cell_id, cell_info->lac, cell_info->arfcn,
               cell_info->bsic, cell_info->timing_advance, cell_info->rx_level,
               cell_info->num_instances);
        rx_lev = cell_info->rx_level;
        bsic = cell_info->bsic;
        cell_id = cell_info->cell_id;
        lac = cell_info->lac;
        arfcn = cell_info->arfcn;
        memcpy(reported_plmn, cell_info->plmn, 3);
        //        bcch = cell_info->bcch;
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_DEBUG,
                 "Cell %u\n"
                 "\t\t Cell ID: %.8x\n"
                 "\t\t LAC: %.4x\n"
                 "\t\t ARFCN: %.4x\n"
                 "\t\t BSIC: %.2x\n"
                 "\t\t RX Level %.4x\n",
                 j, cell_info->nmr_cell_data[j].cell_id,
                 cell_info->nmr_cell_data[j].lac,
                 cell_info->nmr_cell_data[j].arfcn,
                 cell_info->nmr_cell_data[j].bsic,
                 cell_info->nmr_cell_data[j].rx_level);
        }
      } break;
      case NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO: {
        struct nas_lac_umts_extended_info *cell_info =
            (struct nas_lac_umts_extended_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO\n"
               "\t Cell ID: %.4x\n"
               "\t LAC: %.4x\n"
               "\t UARFCN: %.4x\n"
               "\t PSC: %.4x\n"
               "\t RSCP: %i\n"
               "\t ECIO: %i\n"
               "\t SQUAL: %i\n"
               "\t SRX Level: %i\n"
               "\t Instances: %u\n",
               __func__, cell_info->cell_id, cell_info->lac, cell_info->uarfcn,
               cell_info->psc, cell_info->rscp, cell_info->ecio,
               cell_info->squal, cell_info->srx_level,
               cell_info->num_instances);
        // This one has a short cell id
        srx_lev = cell_info->srx_level;
        //        bsic = cell_info->bsic;
        // cell_id = cell_info->cell_id;
        lac = cell_info->lac;
        arfcn = cell_info->uarfcn;
        psc = cell_info->psc;
        memcpy(reported_plmn, cell_info->plmn, 3);
        //        bcch = cell_info->bcch;
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_DEBUG,
                 "\t Cell %u\n"
                 "\t\t UARFCN %.8x\n"
                 "\t\t PSC: %.4x\n"
                 "\t\t RSCP: %i\n"
                 "\t\t ECIO: %i\n"
                 "\t\t SQUAL: %i\n"
                 "\t\t SRX Level: %i\n"
                 "\t\t Rank: %i\n"
                 "\t\t Cell Set: %.2x\n",
                 j, cell_info->umts_monitored_cell_extended_info[j].uarfcn,
                 cell_info->umts_monitored_cell_extended_info[j].psc,
                 cell_info->umts_monitored_cell_extended_info[j].rscp,
                 cell_info->umts_monitored_cell_extended_info[j].ecio,
                 cell_info->umts_monitored_cell_extended_info[j].squal,
                 cell_info->umts_monitored_cell_extended_info[j].srx_level,
                 cell_info->umts_monitored_cell_extended_info[j].rank,
                 cell_info->umts_monitored_cell_extended_info[j].cell_set);
        }
      } break;
      case NAS_CELL_LAC_INFO_SCELL_GERAN_CONF: {
        struct nas_lac_scell_geran_config *cell_info =
            (struct nas_lac_scell_geran_config *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_SCELL_GERAN_CONF\n"
               "\t is PCCH present?: %u\n"
               "\t GPRS Minimum RX Access level: %.2x\n"
               "\t MAX TX Power CCH: %.2x\n",
               __func__, cell_info->is_pbcch_present,
               cell_info->gprs_min_rx_level_access,
               cell_info->max_tx_power_cch);
      } break;
      case NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT: {
        struct nas_lac_current_l1_timeslot *cell_info =
            (struct nas_lac_current_l1_timeslot *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT\n"
               "\t Timeslot: %.2x\n",
               __func__, cell_info->timeslot_num);
      } break;
      case NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ: {
        struct nas_lac_doppler_measurement *cell_info =
            (struct nas_lac_doppler_measurement *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ\n"
               "\t Doppler: %.2x\n",
               __func__, cell_info->doppler_measurement_hz);
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN: {
        struct nas_lac_lte_extended_intra_earfcn_info *cell_info =
            (struct nas_lac_lte_extended_intra_earfcn_info *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN\n"
               "\t EARFCN: %.8x\n",
               __func__, cell_info->earfcn);
        arfcn = cell_info->earfcn;
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN: {
        struct nas_lac_lte_extended_interfrequency_earfcn *cell_info =
            (struct nas_lac_lte_extended_interfrequency_earfcn *)(buf + offset);
        logger(MSG_DEBUG,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN\n"
               "\t EARFCN: %.8x\n",
               __func__, cell_info->earfcn);
        arfcn = cell_info->earfcn;
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN: {
        struct nas_lac_wcdma_extended_lte_neighbour_info_earfcn *cell_info =
            (struct nas_lac_wcdma_extended_lte_neighbour_info_earfcn *)(buf +
                                                                        offset);
        logger(
            MSG_DEBUG,
            "%s: NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN\n"
            "\t Number of items: %u\n",
            __func__, cell_info->num_instances);
        for (uint8_t i = 0; i < cell_info->num_instances; i++) {
          logger(MSG_DEBUG,
                 "\t Type: WCDMA Extended Info: Neighbour LTE EARFCNs\n"
                 "\t Item #%u\n",
                 "\t EARFCN: %.8x\n", i, cell_info->earfcn[i]);
        }
      } break;
      default:
        logger(MSG_INFO, "%s: Unknown event %.2x\n", __func__,
               available_tlvs[i]);
      }
    }
  }


  if (memcmp(nas_runtime.curr_plmn, reported_plmn, 3) != 0) {
    logger(MSG_INFO, "%s: Reported PLMN changed\n", __func__);
    memcpy(nas_runtime.prev_plmn, nas_runtime.curr_plmn, 3);
    memcpy(nas_runtime.curr_plmn, reported_plmn, 3);

    nas_runtime.curr_state.mcc[0] = (nas_runtime.curr_plmn[0] & 0x0f) + 0x30;
    nas_runtime.curr_state.mcc[1] = ((nas_runtime.curr_plmn[0] & 0xf0) >> 4) + 0x30;
    nas_runtime.curr_state.mcc[2] = (nas_runtime.curr_plmn[1] & 0x0f) + 0x30;
    nas_runtime.curr_state.mnc[0] = (nas_runtime.curr_plmn[2] & 0x0f) + 0x30;
    nas_runtime.curr_state.mnc[1] = ((nas_runtime.curr_plmn[2] & 0xf0) >> 4) + 0x30;
  }
  
  process_current_network_data(mcc, mnc, type_of_service, lac, phy_cell_id,
                               cell_id, bsic, bcch, psc, arfcn, srx_lev,
                               rx_lev);

}

void log_cell_location_information(uint8_t *buf, size_t buf_len) {
  uint32_t curr_time = time(NULL);
  if (!is_signal_tracking_enabled()) {
    logger(MSG_DEBUG, "%s: Tracking is disabled\n", __func__);
    return;
  }

  if (did_qmi_op_fail(buf, buf_len) != QMI_RESULT_SUCCESS) {
    logger(MSG_ERROR, "%s: Baseband returned an error to the request \n",
           __func__);
    return;
  }

  uint8_t available_tlvs[] = {
      NAS_CELL_LAC_INFO_UMTS_CELL_INFO,
      NAS_CELL_LAC_INFO_CDMA_CELL_INFO,
      NAS_CELL_LAC_INFO_LTE_INTRA_INFO,
      NAS_CELL_LAC_INFO_LTE_INTER_INFO,
      NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM,
      NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA,
      NAS_CELL_LAC_INFO_UMTS_CELL_ID,
      NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR,
      NAS_CELL_LAC_INFO_CDMA_RX_INFO,
      NAS_CELL_LAC_INFO_HDR_RX_INFO,
      NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED,
      NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED,
      NAS_CELL_LAC_INFO_WCDMA_GSM_NEIGHBOUT_CELL_EXTENDED,
      NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV,
      NAS_CELL_LAC_INFO_WCDMA_INFO_ACTIVE_SET,
      NAS_CELL_LAC_INFO_WCDMA_ACTIVE_SET_REF_RADIO_LINK,
      NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO,
      NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO,
      NAS_CELL_LAC_INFO_WCDMA_EXTENDED_INFO_AS,
      NAS_CELL_LAC_INFO_SCELL_GERAN_CONF,
      NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT,
      NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ,
      NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN,
      NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN,
      NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN,
      NAS_CELL_LAC_INFO_NAS_INFO_EMM_STATE,
      NAS_CELL_LAC_INFO_NAS_RRC_STATE,
      NAS_CELL_LAC_INFO_LTE_INFO_RRC_STATE,
  };

  logger(MSG_DEBUG, "%s: Found %u information segments in this message\n",
         __func__, count_tlvs_in_message(buf, buf_len));
  for (uint8_t i = 0; i < 27; i++) {
    int offset = get_tlv_offset_by_id(buf, buf_len, available_tlvs[i]);
    if (offset > 0) {
      logger(MSG_DEBUG, "%s: TLV %.2x found at offset %.2x\n", __func__,
             available_tlvs[i], offset);
      switch (available_tlvs[i]) {
      case NAS_CELL_LAC_INFO_UMTS_CELL_INFO: {
        char header[] = "Time,"
                        "Cell ID,"
                        "Location Area Code,"
                        "UARFCN,"
                        "PSC,"
                        "RSCP,"
                        "Signal level,"
                        "ECIO,"
                        "Monitored cell count,"
                        "Neighbour,"
                        "UARFCN,"
                        "PSC,"
                        "RSCP,"
                        "ECIO\n";
        struct nas_lac_umts_cell_info *cell_info =
            (struct nas_lac_umts_cell_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_UMTS_CELL_INFO", header,
                     "%u,"
                     "%.4x,"
                     "%.4x,"
                     "%.4x,"
                     "%.4x,"
                     "%i,"
                     "%i,"
                     "%i,"
                     "%u\n",
                     curr_time, cell_info->cell_id, cell_info->lac,
                     cell_info->uarfcn, cell_info->psc, cell_info->rscp,
                     cell_info->ecio, cell_info->instances);

        for (uint8_t j = 0; j < cell_info->instances; j++) {
          dump_to_file("NAS_CELL_LAC_INFO_UMTS_CELL_INFO", header,
                       "%u,"
                       "%.4x,"
                       "%.4x,"
                       "%.4x,"
                       "%.4x,"
                       "%i,"
                       "%i,"
                       "%i,"
                       "%u,"
                       "%u,"
                       "%.4x,"
                       "%.4x,"
                       "%i,"
                       "%i\n",
                       curr_time, cell_info->cell_id, cell_info->lac,
                       cell_info->uarfcn, cell_info->psc, cell_info->rscp,
                       cell_info->ecio, cell_info->instances, j,
                       cell_info->monitored_cells[j].uarfcn,
                       cell_info->monitored_cells[j].psc,
                       cell_info->monitored_cells[j].rscp,
                       cell_info->monitored_cells[j].ecio);
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INTRA_INFO: {
        char header[] = "Time,"
                        "Is Idle,"
                        "Cell ID,"
                        "Tracking Area Code,"
                        "EARFCN,"
                        "Serving Cell ID,"
                        "Serving Frequency Priority,"
                        "Non-Intrafrequency search threshold,"
                        "Serving cell lower threshold,"
                        "Intrafrequency search threshold,"
                        "Neighbour Cell count,"
                        "Neighbour,"
                        "PHY Cell ID,"
                        "RSRQ,"
                        "RSRP,"
                        "RSSI Lev,"
                        "SRX Lev\n";
        struct nas_lac_lte_intra_cell_info *cell_info =
            (struct nas_lac_lte_intra_cell_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_LTE_INTRA_INFO", header,
                     "%u,"
                     "%s,"
                     "%.4x,"
                     "%.4x,"
                     "%.4x,"
                     "%.4x,"
                     "%.2x,"
                     "%.2x,"
                     "%.2x,"
                     "%u\n", 
                     curr_time,
                     (cell_info->is_idle == 1) ? "Yes" : "No",
                     cell_info->cell_id,
                     cell_info->tracking_area_code,
                     cell_info->earfcn, 
                     cell_info->serv_cell_id,
                     cell_info->non_intra_search_threshold,
                     cell_info->serving_cell_lower_threshold,
                     cell_info->intra_search_threshold,
                     cell_info->num_of_cells);

        for (uint8_t j = 0; j < cell_info->num_of_cells; j++) {
          dump_to_file("NAS_CELL_LAC_INFO_LTE_INTRA_INFO", header,
                      "%u,"
                      "%s,"
                      "%.4x,"
                      "%.4x,"
                      "%.4x,"
                      "%.4x,"
                      "%.2x,"
                      "%.2x,"
                      "%.2x,"
                      "%u,"
                       "%u,"
                       "%.4x,"
                       "%.4x,"
                       "%i,"
                       "%i,"
                       "%i\n",
                       curr_time, 
                       (cell_info->is_idle == 1) ? "Yes" : "No",
                       cell_info->cell_id, 
                       cell_info->tracking_area_code,
                       cell_info->earfcn, 
                       cell_info->serv_cell_id,
                       cell_info->non_intra_search_threshold,
                       cell_info->serving_cell_lower_threshold,
                       cell_info->intra_search_threshold,
                       cell_info->num_of_cells,
                       j, 
                       cell_info->lte_cell_info[j].phy_cell_id,
                       cell_info->lte_cell_info[j].rsrq,
                       cell_info->lte_cell_info[j].rsrp,
                       cell_info->lte_cell_info[j].srx_level,
                       cell_info->lte_cell_info[j].rssi_level);
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INTER_INFO: {
        char header[] = "Time,"
                        "Is Idle?,"
                        "Instances,"

                        "Instance,"
                        "EARFCN,"
                        "SRX Level Low Threshold,"
                        "SRX Level High Threshold,"
                        "Reselect Priority,"
                        "Number of cells:,"

                        "Cell count,"
                        "PHY Cell ID,"
                        "RSRQ,"
                        "RSRP,"
                        "RSSI Lev,"
                        "SRX Lev\n";
        struct nas_lac_lte_inter_cell_info *cell_info =
            (struct nas_lac_lte_inter_cell_info *)(buf + offset);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          for (uint8_t k = 0;
               k < cell_info->lte_inter_freq_instance[j].num_cells; k++) {
            dump_to_file(
                "NAS_CELL_LAC_INFO_LTE_INTER_INFO", header,
                "%u," // time
                "%s," // idle
                "%u," // instances
                "%u," // curr instance
                "%.4x," // earfcn
                "%.2x," //srx low
                "%.2x," // srx hi
                "%.2x," // resel prio
                "%i," // num of cells
                "%u," // curr cell (k)
                "%.4x," //phy cell id
                "%.4x," // rsrq
                "%i," // rsrp
                "%i," // rssi lev
                "%i\n", // srxlev
                curr_time, 
                (cell_info->is_idle == 1) ? "Yes" : "No",
                cell_info->num_instances, 
                j,
                cell_info->lte_inter_freq_instance[j].earfcn,
                cell_info->lte_inter_freq_instance[j].srxlev_low_threshold,
                cell_info->lte_inter_freq_instance[j].srxlev_high_threshold,
                cell_info->lte_inter_freq_instance[j].reselect_priority,
                cell_info->lte_inter_freq_instance[j].num_cells,
                k, 
                cell_info->lte_inter_freq_instance[j]
                    .lte_cell_info[k]
                    .phy_cell_id,
                cell_info->lte_inter_freq_instance[j].lte_cell_info[k].rsrq,
                cell_info->lte_inter_freq_instance[j].lte_cell_info[k].rsrp,
                cell_info->lte_inter_freq_instance[j]
                    .lte_cell_info[k]
                    .rssi_level,
                cell_info->lte_inter_freq_instance[j]
                    .lte_cell_info[k]
                    .srx_level);
          }
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM: {
        char header[] = "Time,"
                        "Is Idle?,"
                        "Instances,"
                        "Instance,"
                        "Cell Reselect Priority,"
                        "Reselect threshold (hi),"
                        "Reselect threshold (low),"
                        "NCC mask,"
                        "Number of cells,"
                        "Cell count,"
                        "ARFCN,"
                        "Band,"
                        "Is cell ID valid?,"
                        "BSIC,"
                        "RSSI,"
                        "SRX Lev\n";
        struct nas_lac_lte_gsm_neighbour_info *cell_info =
            (struct nas_lac_lte_gsm_neighbour_info *)(buf + offset);
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          for (uint8_t k = 0; k < cell_info->lte_gsm_neighbours[j].num_cells;
               k++) {
            dump_to_file(
                "NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM", header,
                "%u,"
                "%s,"
                "%u,"
                "%u,"
                "%.2x,"
                "%.2x,"
                "%.2x,"
                "%.2x,"
                "%i,"
                "%u,"
                "%.4x,"
                "%.2x,"
                "%.2x,"
                "%.2x,"
                "%i,"
                "%i\n",
                curr_time, (cell_info->is_idle == 1) ? "Yes" : "No",
                cell_info->num_instances, j,
                cell_info->lte_gsm_neighbours[j].cell_reselect_priority,
                cell_info->lte_gsm_neighbours[j]
                    .high_priority_reselect_threshold,
                cell_info->lte_gsm_neighbours[j].low_prio_reselect_threshold,
                cell_info->lte_gsm_neighbours[j].ncc_mask,
                cell_info->lte_gsm_neighbours[j].num_cells, k,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .arfcn,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].band,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .is_cell_id_valid,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].bsic,
                cell_info->lte_gsm_neighbours[j].lte_gsm_cell_neighbour[k].rssi,
                cell_info->lte_gsm_neighbours[j]
                    .lte_gsm_cell_neighbour[k]
                    .srxlev

            );
          }
        }
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA: {
        char header[] = "Time,"
                        "Is Idle?,"
                        "Instances,"
                        "Instance,"
                        "Cell Reselect Priority,"
                        "Reselect threshold (hi),"
                        "Reselect threshold (low),"
                        "UARFCN,"
                        "Number of cells,"
                        "Cell count,"
                        "PSC,"
                        "CPICH_RSCP,"
                        "CPICH_ECNO,"
                        "SRX Level\n";
        struct nas_lac_lte_wcdma_neighbour_info *cell_info =
            (struct nas_lac_lte_wcdma_neighbour_info *)(buf + offset);
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          for (uint8_t k = 0; k < cell_info->lte_wcdma_neighbours[j].num_cells;
               k++) {
            dump_to_file(
                "NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA", header,
                "%u,"
                "%s,"
                "%u,"
                "%u,"
                "%.2x,"
                "%.4x,"
                "%.4x,"
                "%.4x,"
                "%i,"
                "%u,"
                "%.4x,"
                "%.2x,"
                "%.2x,"
                "%.2x\n",
                curr_time, (cell_info->is_idle == 1) ? "Yes" : "No",
                cell_info->num_instances, j,
                cell_info->lte_wcdma_neighbours[j].cell_reselect_priority,
                cell_info->lte_wcdma_neighbours[j]
                    .high_priority_reselect_threshold,
                cell_info->lte_wcdma_neighbours[j].low_prio_reselect_threshold,
                cell_info->lte_wcdma_neighbours[j].uarfcn,
                cell_info->lte_wcdma_neighbours[j].num_cells, k,
                cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].psc,
                cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].cpich_rscp,
                cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].cpich_ecno,
                cell_info->lte_wcdma_neighbours[j].wcdma_cells[k].srx_level);
          }
        }
      }

      break;
      case NAS_CELL_LAC_INFO_UMTS_CELL_ID: {
        char header[] = "Time, Type, Cell ID\n";
        struct nas_lac_umts_cell_id *cell_info =
            (struct nas_lac_umts_cell_id *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_UMTS_CELL_ID", header,
                     "%u, UMTS,"
                     "%.4x\n",
                     curr_time, cell_info->cell_id);
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR: {
        char header[] = "Time,"
                        "RRC State,"
                        "Number of cells,"
                        "Cell,"
                        "EARFCN,"
                        "Physical Cell ID,"
                        "RSRP,"
                        "RSRQ,"
                        "SRX Level,"
                        "Is cell TDD?\n";
        struct nas_lac_wcdma_lte_neighbour_info *cell_info =
            (struct nas_lac_wcdma_lte_neighbour_info *)(buf + offset);
        for (uint8_t j = 0; j < cell_info->num_cells; j++) {
          dump_to_file("NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR", header,
                       "%u,"
                       "%.4x,"
                       "%u,"
                       "%u,"
                       "%.4x,"
                       "%.4x,"
                       "%.4x,"
                       "%.4x,"
                       "%i,"
                       "%i\n",
                       curr_time, cell_info->rrc_state, cell_info->num_cells, j,
                       cell_info->wcdma_lte_neighbour[j].earfcn,
                       cell_info->wcdma_lte_neighbour[j].phy_cell_id,
                       cell_info->wcdma_lte_neighbour[j].rsrp,
                       cell_info->wcdma_lte_neighbour[j].rsrq,
                       cell_info->wcdma_lte_neighbour[j].srx_level,
                       cell_info->wcdma_lte_neighbour[j].is_cell_tdd);
        }
      } break;
      case NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED: {
        char header[] = "Time, Timing Advance, Channel Freq.\n";
        struct nas_lac_extended_gsm_cell_info *cell_info =
            (struct nas_lac_extended_gsm_cell_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED", header,
                     "%u,"
                     "%.4x,"
                     "%.4x\n",
                     curr_time, cell_info->timing_advance, cell_info->bcch);
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED: {
        char header[] = "Time,"
                        "WCDMA Power (dB),"
                        "WCDMA TX Power (dB),"
                        "Downlink Error rate\n";
        struct nas_lac_extended_wcdma_cell_info *cell_info =
            (struct nas_lac_extended_wcdma_cell_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED", header,
                     "%u,"
                     "%.4x,"
                     "%.4x,"
                     "%i %%\n",
                     curr_time, cell_info->wcdma_power_db,
                     cell_info->wcdma_tx_power_db,
                     cell_info->downlink_error_rate);

      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV: {
        struct nas_lac_lte_timing_advance_info *cell_info =
            (struct nas_lac_lte_timing_advance_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV",
                     "Time, Timing advance\n", "%u, %i\n", curr_time,
                     cell_info->timing_advance);
      } break;
      case NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO: {
        char header[] = "Time,"
                        "Cell ID,"
                        "LAC,"
                        "ARFCN,"
                        "BSIC,"
                        "Timing advance,"
                        "RX Level,"
                        "Number of instances,"
                        "Cell,"
                        "Cell ID,"
                        "LAC,"
                        "ARFCN,"
                        "BSIC,"
                        "RX Level\n";
        struct nas_lac_extended_geran_info *cell_info =
            (struct nas_lac_extended_geran_info *)(buf + offset);
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          dump_to_file(
              "NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO", header,
              "%u,"
              "%.8x,"
              "%.4x,"
              "%.4x,"
              "%.2x,"
              "%i,"
              "%.4x,"
              "%u,"
              "%u,"
              "%.8x,"
              "%.4x,"
              "%.4x,"
              "%.2x,"
              "%.4x\n",
              curr_time, cell_info->cell_id, cell_info->lac, cell_info->arfcn,
              cell_info->bsic, cell_info->timing_advance, cell_info->rx_level,
              cell_info->num_instances, j, cell_info->nmr_cell_data[j].cell_id,
              cell_info->nmr_cell_data[j].lac,
              cell_info->nmr_cell_data[j].arfcn,
              cell_info->nmr_cell_data[j].bsic,
              cell_info->nmr_cell_data[j].rx_level);
        }
      } break;
      case NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO: {
        char header[] = "Time,"
                        "Cell ID,"
                        "LAC,"
                        "UARFCN,"
                        "PSC,"
                        "RSCP,"
                        "ECIO,"
                        "SQUAL,"
                        "SRX Level,"
                        "Instances,"
                        "Cell,"
                        "UARFCN,"
                        "PSC,"
                        "RSCP,"
                        "ECIO,"
                        "SQUAL,"
                        "SRX Level,"
                        "Rank,"
                        "Cell Set\n";
        struct nas_lac_umts_extended_info *cell_info =
            (struct nas_lac_umts_extended_info *)(buf + offset);
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          dump_to_file(
              "NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO", header,
              "%u,"
              "%.4x,"
              "%.4x,"
              "%.4x,"
              "%.4x,"
              "%i,"
              "%i,"
              "%i,"
              "%i,"
              "%u,"
              "%u,"
              "%.8x,"
              "%.4x,"
              "%i,"
              "%i,"
              "%i,"
              "%i,"
              "%i,"
              "%.2x\n",
              curr_time, cell_info->cell_id, cell_info->lac, cell_info->uarfcn,
              cell_info->psc, cell_info->rscp, cell_info->ecio,
              cell_info->squal, cell_info->srx_level, cell_info->num_instances,
              j, cell_info->umts_monitored_cell_extended_info[j].uarfcn,
              cell_info->umts_monitored_cell_extended_info[j].psc,
              cell_info->umts_monitored_cell_extended_info[j].rscp,
              cell_info->umts_monitored_cell_extended_info[j].ecio,
              cell_info->umts_monitored_cell_extended_info[j].squal,
              cell_info->umts_monitored_cell_extended_info[j].srx_level,
              cell_info->umts_monitored_cell_extended_info[j].rank,
              cell_info->umts_monitored_cell_extended_info[j].cell_set);
        }
      } break;
      case NAS_CELL_LAC_INFO_SCELL_GERAN_CONF: {
        char header[] = "Time,"
                        "Is PCCH present?,"
                        "GPRS Minimum RX Access level,"
                        "MAX TX Power CCH\n,";
        struct nas_lac_scell_geran_config *cell_info =
            (struct nas_lac_scell_geran_config *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_SCELL_GERAN_CONF", header,
                     "%u,"
                     "%u,"
                     "%.2x,"
                     "%.2x\n",
                     curr_time, cell_info->is_pbcch_present,
                     cell_info->gprs_min_rx_level_access,
                     cell_info->max_tx_power_cch);
      } break;
      case NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT: {
        struct nas_lac_current_l1_timeslot *cell_info =
            (struct nas_lac_current_l1_timeslot *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT",
                     "Time, Timeslot\n", "%u, %.2x\n", curr_time,
                     cell_info->timeslot_num);
      } break;
      case NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ: {
        struct nas_lac_doppler_measurement *cell_info =
            (struct nas_lac_doppler_measurement *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ",
                     "Time, Doppler measur.(hz)\n", "%u, %.2x\n", curr_time,
                     cell_info->doppler_measurement_hz);
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN: {
        struct nas_lac_lte_extended_intra_earfcn_info *cell_info =
            (struct nas_lac_lte_extended_intra_earfcn_info *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN",
                     "Time, EARFCN\n", "%u, %.8x\n", curr_time,
                     cell_info->earfcn);
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN: {
        struct nas_lac_lte_extended_interfrequency_earfcn *cell_info =
            (struct nas_lac_lte_extended_interfrequency_earfcn *)(buf + offset);
        dump_to_file("NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN",
                     "Time, EARFCN\n", "%u, %.8x\n", curr_time,
                     cell_info->earfcn);
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN: {
        char header[] = "Time,"
                        "Number of items,"
                        "Item #,"
                        "EARFCN: %.8x\n";
        struct nas_lac_wcdma_extended_lte_neighbour_info_earfcn *cell_info =
            (struct nas_lac_wcdma_extended_lte_neighbour_info_earfcn *)(buf +
                                                                        offset);

        for (uint8_t i = 0; i < cell_info->num_instances; i++) {
          dump_to_file(
              "NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN",
              header,
              "%u,"
              "%u,"
              "%u,",
              "%.8x\n", curr_time, cell_info->num_instances, i,
              cell_info->earfcn[i]);
        }
      } break;
      default:
        logger(MSG_INFO, "%s: Unknown event %.2x\n", __func__,
               available_tlvs[i]);
        break;
      }
    }
  }
}

void nas_update_network_data(uint8_t network_type, uint8_t signal_level) {
  logger(MSG_DEBUG, "%s: update network data\n", __func__);
  nas_runtime.prev_state = nas_runtime.curr_state;
  nas_runtime.curr_state.signal_level = 0;
  switch (network_type) {
  case NAS_SIGNAL_REPORT_TYPE_CDMA:     // CDMA
  case NAS_SIGNAL_REPORT_TYPE_CDMA_HDR: // QCOM HDR CDMA
    nas_runtime.curr_state.network_type = 1;
    break;
  case NAS_SIGNAL_REPORT_TYPE_GSM: // GSM
    nas_runtime.curr_state.network_type = 4;
    break;
  case NAS_SIGNAL_REPORT_TYPE_WCDMA: // WCDMA
    nas_runtime.curr_state.network_type = 5;
    break;
  case NAS_SIGNAL_REPORT_TYPE_LTE: // LTE
    nas_runtime.curr_state.network_type = 8;
    break;
  default:
    logger(MSG_DEBUG, "Unknown signal report: %u\n", network_type);
    nas_runtime.curr_state.network_type = 0;
    break;
  }

  if (signal_level > 0)
    nas_runtime.curr_state.signal_level = signal_level / 2;

  if (is_signal_tracking_enabled()) {
    if (nas_runtime.curr_state.network_type <
        nas_runtime.prev_state.network_type) {
      uint8_t reply[MAX_MESSAGE_SIZE] = {0};
      size_t strsz =
          snprintf((char *)reply, MAX_MESSAGE_SIZE,
                   "Warning: Network service has been downgraded!\n");
      add_message_to_queue(reply, strsz);
    }
  }
}
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_nas_message(uint8_t *buf, size_t buf_len) {
#ifdef DEBUG_NAS
  pretty_print_qmi_pkt("NAS: Baseband --> Host", buf, buf_len);
#endif
  switch (get_qmi_message_id(buf, buf_len)) {
  case NAS_SERVICE_PROVIDER_NAME:
    logger(MSG_INFO, "%s: Service Provider Name\n", __func__);
    break;
  case NAS_OPERATOR_PLMN_LIST:
    logger(MSG_INFO, "%s: Operator PLMN List\n", __func__);
    break;
  case NAS_OPERATOR_PLMN_NAME:
    logger(MSG_INFO, "%s: Operator PLMN Name\n", __func__);
    break;
  case NAS_OPERATOR_STRING_NAME:
    logger(MSG_INFO, "%s: Operator String Name\n", __func__);
    break;
  case NAS_NITZ_INFORMATION:
    logger(MSG_INFO, "%s: NITZ Information\n", __func__);
    break;
  case NAS_SET_EVENT_REPORT:
    logger(MSG_INFO, "%s: Set Event Report\n", __func__);
    break;
  case NAS_REGISTER_INDICATIONS:
    logger(MSG_INFO, "%s: Register Indications\n", __func__);
    break;
  case NAS_GET_SUPPORTED_MESSAGES:
    logger(MSG_INFO, "%s: Get Supported Messages\n", __func__);
    break;
  case NAS_GET_SIGNAL_STRENGTH:
    logger(MSG_INFO, "%s: Get Signal Strength\n", __func__);
    break;
  case NAS_NETWORK_SCAN:
    logger(MSG_INFO, "%s: Network Scan\n", __func__);
    break;
  case NAS_INITIATE_NETWORK_REGISTER:
    logger(MSG_INFO, "%s: Initiate Network Register\n", __func__);
    break;
  case NAS_ATTACH_DETACH:
    logger(MSG_INFO, "%s: Attach Detach\n", __func__);
    break;
  case NAS_GET_SERVING_SYSTEM:
    logger(MSG_INFO, "%s: Get Serving System\n", __func__);
    parse_serving_system_message(buf, buf_len);
    break;
  case NAS_GET_HOME_NETWORK:
    logger(MSG_INFO, "%s: Get Home Network\n", __func__);
    break;
  case NAS_GET_PREFERRED_NETWORKS:
    logger(MSG_INFO, "%s: Get Preferred Networks\n", __func__);
    break;
  case NAS_SET_PREFERRED_NETWORKS:
    logger(MSG_INFO, "%s: Set Preferred Networks\n", __func__);
    break;
  case NAS_SET_TECHNOLOGY_PREFERENCE:
    logger(MSG_INFO, "%s: Set Technology Preference\n", __func__);
    break;
  case NAS_GET_TECHNOLOGY_PREFERENCE:
    logger(MSG_INFO, "%s: Get Technology Preference\n", __func__);
    break;
  case NAS_GET_RF_BAND_INFORMATION:
    logger(MSG_INFO, "%s: Get RF Band Information\n", __func__);
    break;
  case NAS_SET_SYSTEM_SELECTION_PREFERENCE:
    logger(MSG_INFO, "%s: Set System Selection Preference\n", __func__);
    break;
  case NAS_GET_SYSTEM_SELECTION_PREFERENCE:
    logger(MSG_INFO, "%s: Get System Selection Preference\n", __func__);
    break;
  case NAS_GET_OPERATOR_NAME:
    logger(MSG_INFO, "%s: Get Operator Name\n", __func__);
    break;
  case NAS_OPERATOR_NAME:
    update_operator_name(buf, buf_len);
    break;
  case NAS_GET_CELL_LOCATION_INFO:
    logger(MSG_DEBUG, "%s: Get Cell Location Info\n", __func__);
    update_cell_location_information(buf, buf_len);
    if (get_dump_network_tables_config())
      log_cell_location_information(buf, buf_len);
    break;
  case NAS_GET_PLMN_NAME:
    logger(MSG_INFO, "%s: Get PLMN Name\n", __func__);
    break;
  case NAS_NETWORK_TIME:
    logger(MSG_INFO, "%s: Network Time\n", __func__);
    break;
  case NAS_GET_SYSTEM_INFO:
    logger(MSG_INFO, "%s: Get System Info\n", __func__);
    break;
  case NAS_SYSTEM_INFO:
    logger(MSG_INFO, "%s: System Info\n", __func__);
    break;
  case NAS_GET_SIGNAL_INFO:
    logger(MSG_DEBUG, "%s: Get Signal Info\n", __func__);
    struct nas_signal_lev *level = (struct nas_signal_lev *)buf;
    nas_update_network_data(level->signal.id, level->signal.signal_level);
    if (is_first_boot()) {
      send_hello_world();
    }
    level = NULL;
    break;
  case NAS_CONFIG_SIGNAL_INFO:
    logger(MSG_INFO, "%s: Config Signal Info\n", __func__);
    break;
  case NAS_CONFIG_SIGNAL_INFO_V2:
    logger(MSG_INFO, "%s: Config Signal Info v2\n", __func__);
    break;
  case NAS_SIGNAL_INFO:
    logger(MSG_INFO, "%s: Signal Info\n", __func__);
    break;
  case NAS_GET_TX_RX_INFO:
    logger(MSG_INFO, "%s: Get Tx Rx Info\n", __func__);
    break;
  case NAS_GET_CDMA_POSITION_INFO:
    logger(MSG_INFO, "%s: Get CDMA Position Info\n", __func__);
    break;
  case NAS_FORCE_NETWORK_SEARCH:
    logger(MSG_INFO, "%s: Force Network Search\n", __func__);
    break;
  case NAS_NETWORK_REJECT:
    logger(MSG_INFO, "%s: Network Reject\n", __func__);
    break;
  case NAS_GET_DRX:
    logger(MSG_INFO, "%s: Get DRX\n", __func__);
    break;
  case NAS_GET_LTE_CPHY_CA_INFO:
    logger(MSG_INFO, "%s: Get LTE Cphy CA Info\n", __func__);
    break;
  case NAS_SWI_GET_STATUS:
    logger(MSG_INFO, "%s: Swi Get Status\n", __func__);
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for the NAS Service: %.4x\n",
           __func__, get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}

void *register_to_nas_service() {
  load_report_data();
  nas_register_to_events();
  nas_get_ims_preference();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}
