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
#define MAX_REPORT_NUM 4096
#define MAX_FILE_SIZE 13107200

struct basic_network_status {
  uint8_t operator_name[32];
  uint8_t mcc[4];
  uint8_t mnc[3];
  uint16_t location_area_code_1;
  uint16_t location_area_code_2;
  struct service_capability service_capability;
  uint8_t network_registration_status;
  uint8_t circuit_switch_attached;
  uint8_t packet_switch_attached;
  uint8_t radio_access;
};

struct {
  /* Basic state */
  struct basic_network_status current_state;
  /* from cell.h */
  struct network_state current_network_state;

  struct basic_network_status previous_state;
  struct network_state previous_network_state;

  /* Network status report history */
  struct network_status_reports data[MAX_REPORT_NUM];
  uint16_t current_report;

  /* Open Cellid data */
  uint8_t cellid_data_missing; // 0 missing, 1 ready, 2 missing, requested
  void *open_cellid_import;
  uint64_t open_cellid_num_items;
  uint8_t open_cellid_mcc[4];
  uint8_t open_cellid_mnc[3];

} nas_runtime;

void get_opencellid_data() {
  FILE *fp;
  char filename[256];
  size_t filesize = 0;
  snprintf(filename, 255, "/tmp/%s-%s.bin",
           (char *)nas_runtime.current_state.mcc,
           (char *)nas_runtime.current_state.mnc);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    logger(
        MSG_WARN,
        "%s: Can't find OpenCellid database for the current carrier: %s-%s\n",
        __func__, nas_runtime.current_state.mcc, nas_runtime.current_state.mnc);
    return;
  }

  if (nas_runtime.open_cellid_import) {
    /* Clear out the previous file */
    free(nas_runtime.open_cellid_import);
  }
  fseek(fp, 0L, SEEK_END);
  filesize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  if (filesize > MAX_FILE_SIZE) {
    logger(MSG_ERROR, "%s: File is too big\n", __func__);
  } else {
    nas_runtime.open_cellid_import = malloc(filesize);
    nas_runtime.open_cellid_num_items =
        filesize / sizeof(struct ocid_cell_slim);
    if (fread(nas_runtime.open_cellid_import, filesize, 1, fp) != 1) {
      logger(MSG_ERROR, "%s: Error reading OpenCellid database!\n", __func__);
      fclose(fp);
      free(nas_runtime.open_cellid_import);
      return;
    }
    memcpy(nas_runtime.open_cellid_mcc, nas_runtime.current_state.mcc, 4);
    memcpy(nas_runtime.open_cellid_mnc, nas_runtime.current_state.mnc, 3);
    nas_runtime.cellid_data_missing = 1;
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "OpenCellid database is ready!");
    add_message_to_queue(reply, strsz);
  }
  logger(MSG_INFO, "%s: End\n", __func__);
  fclose(fp);
}

int is_cell_id_in_db(uint32_t cell_id, uint16_t lac) {
  if (nas_runtime.cellid_data_missing != 1) {
    logger(MSG_ERROR, "%s: Open Cellid data isn't available\n", __func__);
    return -EINVAL;
  }
  logger(MSG_INFO, "%s Looking for the cell id\n", __func__);
  for (uint64_t i = 0; i < nas_runtime.open_cellid_num_items; i++) {
    struct ocid_cell_slim *ocid =
        (nas_runtime.open_cellid_import + (i * sizeof(struct ocid_cell_slim)));
    if (cell_id == ocid->cell && ocid->area == lac) {
      logger(MSG_INFO, "%s: Found %u %u (OpenCellID: %u %u)\n", __func__,
             cell_id, lac, ocid->cell, ocid->area);
      return 1;
    } else if (ocid->area == lac) {
    }
  }
  logger(MSG_INFO, "%s: Didn't find it... :(\n ", __func__);
  return 0;
}

uint8_t is_cellid_data_missing() {
  if (nas_is_network_in_service())
    return nas_runtime.cellid_data_missing; 
  return 2;
  }

void set_cellid_data_missing_as_requested() {
  nas_runtime.cellid_data_missing = 2;
      uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "OpenCellID database is not available. Please check https://cid.themodemdistro.com for more info");
    add_message_to_queue(reply, strsz);
}

uint8_t *get_current_mcc() { return nas_runtime.current_state.mcc; }
uint8_t *get_current_mnc() { return nas_runtime.current_state.mnc; }
uint8_t get_network_type() {
  return nas_runtime.current_network_state.network_type;
}


struct network_state get_network_status() {
  return nas_runtime.current_network_state;
}

/* Used in command.c */
struct nas_report get_current_cell_report() {
  return nas_runtime.data[nas_runtime.current_report].report;
}

/* Returns last reported signal in %, based on signal bars */
uint8_t get_signal_strength() {
  return nas_runtime.current_network_state.signal_level;
}

uint8_t nas_is_network_in_service() {
  if (nas_runtime.current_state.service_capability.gprs ||
      nas_runtime.current_state.service_capability.edge ||
      nas_runtime.current_state.service_capability.hsdpa ||
      nas_runtime.current_state.service_capability.hsupa ||
      nas_runtime.current_state.service_capability.wcdma ||
      nas_runtime.current_state.service_capability.gsm ||
      nas_runtime.current_state.service_capability.lte ||
      nas_runtime.current_state.service_capability.hsdpa_plus ||
      nas_runtime.current_state.service_capability.dc_hsdpa_plus)
    return 1;

  return 0;
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

int nas_get_signal_info() {
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
    memcpy(nas_runtime.current_state.operator_name, name->operator_name,
           name->len - (sizeof(uint16_t)));
  }
  offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
  if (offset > 0) {
    struct carrier_mcc_mnc *op_code = (struct carrier_mcc_mnc *)(buf + offset);
    if (buf_len < op_code->len) {
      logger(MSG_ERROR, "%s: Message is shorter than carrier data\n", __func__);
      return;
    }
    memcpy(nas_runtime.current_state.mcc, op_code->mcc, 3);
    memcpy(nas_runtime.current_state.mnc, op_code->mnc, 2);
    nas_runtime.current_state.location_area_code_1 = op_code->lac1;
    nas_runtime.current_state.location_area_code_2 = op_code->lac2;
  }

  logger(MSG_INFO,
         "%s: Report\n"
         "\tOperator: %s\n"
         "\tMCC-MNC: %s-%s\n"
         "\tLACs: %.4x %.4x\n",
         __func__, nas_runtime.current_state.operator_name,
         (unsigned char *)nas_runtime.current_state.mcc,
         (unsigned char *)nas_runtime.current_state.mnc,
         nas_runtime.current_state.location_area_code_1,
         nas_runtime.current_state.location_area_code_2);
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
    nas_runtime.previous_state = nas_runtime.current_state;

    // Set everything to 0, then repopulate
    nas_runtime.current_state.service_capability.gprs = 0;
    nas_runtime.current_state.service_capability.edge = 0;
    nas_runtime.current_state.service_capability.hsdpa = 0;
    nas_runtime.current_state.service_capability.hsupa = 0;
    nas_runtime.current_state.service_capability.wcdma = 0;
    nas_runtime.current_state.service_capability.gsm = 0;
    nas_runtime.current_state.service_capability.lte = 0;
    nas_runtime.current_state.service_capability.hsdpa_plus = 0;
    nas_runtime.current_state.service_capability.dc_hsdpa_plus = 0;

    for (uint16_t i = 0; i < capability_arr->len; i++) {
      switch (capability_arr->data[i]) {
      case 0x00: // When is not yet connected to the network
        logger(MSG_INFO, "%s: No service\n", __func__);
        break;
      case 0x01:
        nas_runtime.current_state.service_capability.gprs = 1;
        break;
      case 0x02:
        nas_runtime.current_state.service_capability.edge = 1;
        break;
      case 0x03:
        nas_runtime.current_state.service_capability.hsdpa = 1;
        break;
      case 0x04:
        nas_runtime.current_state.service_capability.hsupa = 1;
        break;
      case 0x05:
        nas_runtime.current_state.service_capability.wcdma = 1;
        break;
      case 0x0a:
        nas_runtime.current_state.service_capability.gsm = 1;
        break;
      case 0x0b:
        nas_runtime.current_state.service_capability.lte = 1;
        break;
      case 0x0c:
        nas_runtime.current_state.service_capability.hsdpa_plus = 1;
        break;
      case 0x0d:
        nas_runtime.current_state.service_capability.dc_hsdpa_plus = 1;
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
    nas_runtime.current_state.network_registration_status =
        serving_sys->registration_status;
    nas_runtime.current_state.circuit_switch_attached =
        serving_sys->cs_attached;
    nas_runtime.current_state.packet_switch_attached = serving_sys->ps_attached;
    nas_runtime.current_state.radio_access = serving_sys->radio_access;
  }
  logger(
      MSG_INFO,
      "%s: Previous capabilities\n"
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
      nas_runtime.previous_state.service_capability.gsm == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.gprs == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.edge == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.wcdma == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.hsdpa == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.hsupa == 1 ? "Yes" : "No",
      nas_runtime.previous_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                    : "No",
      nas_runtime.previous_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                       : "No",
      nas_runtime.previous_state.service_capability.lte == 1 ? "Yes" : "No",
      nas_runtime.previous_state.circuit_switch_attached == 1 ? "Yes" : "No",
      nas_runtime.previous_state.packet_switch_attached == 1 ? "Yes" : "No");

  logger(MSG_INFO,
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
         nas_runtime.current_state.service_capability.gsm == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.gprs == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.edge == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.wcdma == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.hsdpa == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.hsupa == 1 ? "Yes" : "No",
         nas_runtime.current_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                      : "No",
         nas_runtime.current_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                         : "No",
         nas_runtime.current_state.service_capability.lte == 1 ? "Yes" : "No",
         nas_runtime.current_state.circuit_switch_attached == 1 ? "Yes" : "No",
         nas_runtime.current_state.packet_switch_attached == 1 ? "Yes" : "No");

  if (nas_runtime.previous_state.packet_switch_attached &&
      !nas_runtime.previous_state.packet_switch_attached) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "NASdbg: Packet switch detached!");
    add_message_to_queue(reply, strsz);
  }
  if (nas_runtime.previous_state.circuit_switch_attached &&
      !nas_runtime.previous_state.circuit_switch_attached) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                            "NASdbg: Circuit switch detached!");
    add_message_to_queue(reply, strsz);
  }
  if (nas_runtime.previous_state.service_capability.lte &&
      !nas_runtime.previous_state.service_capability.lte) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "Service Capability changed:\n"
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
        nas_runtime.current_state.service_capability.gsm == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.gprs == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.edge == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.wcdma == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsdpa == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsupa == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                     : "No",
        nas_runtime.current_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                        : "No",
        nas_runtime.current_state.service_capability.lte == 1 ? "Yes" : "No",
        nas_runtime.current_state.circuit_switch_attached == 1 ? "Yes" : "No",
        nas_runtime.current_state.packet_switch_attached == 1 ? "Yes" : "No");
    add_message_to_queue(reply, strsz);
  }
}

void process_current_network_data(uint16_t mcc, uint16_t mnc,
                                  uint8_t type_of_service, uint16_t lac,
                                  uint16_t phy_cell_id, uint32_t cell_id,
                                  uint8_t bsic, uint16_t bcch, uint16_t psc,
                                  uint16_t arfcn, int16_t srx_lev,
                                  uint16_t rx_lev) {

  logger(MSG_INFO, "%s: Start\n", __func__);
  if (lac == 0 || cell_id == 0) {
    logger(MSG_WARN, "%s: Not enough data to process\n", __func__);
    return;
  }
  /* Do stuff here */
  uint8_t lac_is_known = 0;
  uint8_t cell_is_known = 0;
  int report_id = 0;
  for (int i = 0; i < MAX_REPORT_NUM; i++) {
    if (nas_runtime.data[i].report.lac == lac) {
      lac_is_known = 1;
    }
  }
  if (!lac_is_known) {
    // check if lac is in db, add lac to known dbs
  }

  for (int i = 0; i < MAX_REPORT_NUM; i++) {
    if (nas_runtime.data[i].report.cell_id == cell_id &&
        nas_runtime.data[i].report.lac == lac) {
      cell_is_known = 1;
      report_id = i;
    }
  }
  // Check against opencellid
  if (!cell_is_known) {
    nas_runtime.current_report++;
    if (nas_runtime.current_report > MAX_REPORT_NUM) {
      logger(MSG_INFO, "%s: Rotating log...\n", __func__);
      for (int k = 1; k < MAX_REPORT_NUM; k++) {
        nas_runtime.data[k - 1] = nas_runtime.data[k];
      }
    }
    logger(MSG_INFO, "%s: Storing report with ID %i\n", __func__, nas_runtime.current_report);
    report_id = nas_runtime.current_report;
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
  }

  if (!nas_runtime.data[report_id].report.opencellid_verified) {
    if (is_cell_id_in_db(cell_id, lac) == 1) {
      nas_runtime.data[report_id].report.opencellid_verified = 1;
      uint8_t reply[MAX_MESSAGE_SIZE] = {0};
      size_t strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                              "Nettrack: Verified Cell ID %.8x with LAC %.4x",
                              cell_id, lac);
      add_message_to_queue(reply, strsz);
    } else if (is_cell_id_in_db(cell_id, lac) == 0) {
      uint8_t reply[MAX_MESSAGE_SIZE] = {0};
      size_t strsz =
          snprintf((char *)reply, MAX_MESSAGE_SIZE,
                   "Nettrack: WARNING: Can't verify Cell ID %.8x with LAC %.4x",
                   cell_id, lac);
      add_message_to_queue(reply, strsz);
    }
  }

  logger(MSG_INFO, "%s: End\n", __func__);
}

void update_cell_location_information(uint8_t *buf, size_t buf_len) {
  uint16_t mcc, mnc, lac, phy_cell_id, bcch, psc, arfcn, srx_lev, rx_lev = 0;
  uint8_t type_of_service, bsic = 0;
  uint32_t cell_id = 0;

  if (did_qmi_op_fail(buf, buf_len) != QMI_RESULT_SUCCESS) {
    logger(MSG_ERROR, "%s: Baseband returned an error to the request \n",
           __func__);
    return;
  }
  nas_runtime.current_report++;
  if (nas_runtime.current_report > MAX_REPORT_NUM) {
    nas_runtime.current_report = 0;
  }
  // Not sure if this will change fast enough
  if (memcmp(nas_runtime.current_state.mcc, nas_runtime.open_cellid_mcc, 4) !=
          0 ||
      memcmp(nas_runtime.current_state.mnc, nas_runtime.open_cellid_mnc, 3) !=
          0) {
    logger(MSG_INFO, "%s: Needs OpenCellid data refresh!\n", __func__);
    get_opencellid_data();
  }
  mcc = atoi((char *)nas_runtime.current_state.mcc);
  mnc = atoi((char *)nas_runtime.current_state.mnc);

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
  logger(MSG_INFO, "%s: Found %u information segments in this message\n",
         __func__, count_tlvs_in_message(buf, buf_len));
  for (uint8_t i = 0; i < 27; i++) {
    int offset = get_tlv_offset_by_id(buf, buf_len, available_tlvs[i]);
    if (offset > 0) {
      logger(MSG_INFO, "%s: TLV %.2x found at offset %.2x\n", __func__,
             available_tlvs[i], offset);
      switch (available_tlvs[i]) {
      case NAS_CELL_LAC_INFO_UMTS_CELL_INFO: {
        struct nas_lac_umts_cell_info *cell_info =
            (struct nas_lac_umts_cell_info *)(buf + offset);
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_UMTS_CELL_INFO\n"
               "Cell ID: %.4x\n" // this one is odd
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

        for (uint8_t j = 0; j < cell_info->instances; j++) {
          logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INTRA_INFO\n"
               "\t Is Idle?: %s\n"
               "\t Cell ID: %.4x\n"
               "\t Tracking Area Code: %.4x\n"
               "\t EARFCN: %.4x\n"
               "\t Serving Cell ID: %.4x\n"
               "\t Serving Frequency Priority: %.2x\n"
               "\t Interfrequency search threshold: %.2x\n"
               "\t Serving cell lower threshold: %.2x\n"
               "\t Reselect threshold: %.2x\n"
               "\t Neighbour Cell count: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->cell_id, cell_info->tracking_area_code,
               cell_info->earfcn, cell_info->serv_cell_id,
               cell_info->serving_freq_prio,
               cell_info->inter_frequency_search_threshold,
               cell_info->serving_cell_lower_threshold,
               cell_info->reselect_threshold, cell_info->num_of_cells);

        type_of_service = OCID_RADIO_LTE;
        cell_id = cell_info->cell_id;
        lac = cell_info->tracking_area_code;
        arfcn = cell_info->earfcn;
        // cell_info->serv_cell_id; // FIXME: We seem to have cell id in u16 and
        // u32 values...

        for (uint8_t j = 0; j < cell_info->num_of_cells; j++) {
          logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INTER_INFO\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_INFO,
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
            logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(
              MSG_INFO,
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
                MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA\n"
               "\t Is Idle?: %s\n"
               "\t Instances: %u\n",
               __func__, (cell_info->is_idle == 1) ? "Yes" : "No",
               cell_info->num_instances);

        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_INFO,
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
            logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: Cell ID\n"
               "\t Type: UMTS\n"
               "\t Cell ID: %.4x\n",
               __func__, cell_info->cell_id);
        cell_id = cell_info->cell_id;
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR: {
        struct nas_lac_wcdma_lte_neighbour_info *cell_info =
            (struct nas_lac_wcdma_lte_neighbour_info *)(buf + offset);
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR\n"
               "\t RRC State: %.4x\n"
               "\t Number of cells: %u\n",
               __func__, cell_info->rrc_state, cell_info->num_cells);

        for (uint8_t j = 0; j < cell_info->num_cells; j++) {
          logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED\n"
               "\t Timing Advance: %.4x\n"
               "\t Channel Freq.: %.4x\n",
               __func__, cell_info->timing_advance, cell_info->bcch);
        bcch = cell_info->bcch;
      } break;
      case NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED: {
        struct nas_lac_extended_wcdma_cell_info *cell_info =
            (struct nas_lac_extended_wcdma_cell_info *)(buf + offset);
        logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV\n"
               "\t Timing advance: %i\n",
               __func__, cell_info->timing_advance);
      } break;
      case NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO: {
        struct nas_lac_extended_geran_info *cell_info =
            (struct nas_lac_extended_geran_info *)(buf + offset);
        logger(MSG_INFO,
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
        //        bcch = cell_info->bcch;
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_INFO,
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
        logger(MSG_INFO,
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
        //        bcch = cell_info->bcch;
        for (uint8_t j = 0; j < cell_info->num_instances; j++) {
          logger(MSG_INFO,
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
        logger(MSG_INFO,
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
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT\n"
               "\t Timeslot: %.2x\n",
               __func__, cell_info->timeslot_num);
      } break;
      case NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ: {
        struct nas_lac_doppler_measurement *cell_info =
            (struct nas_lac_doppler_measurement *)(buf + offset);
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ\n"
               "\t Doppler: %.2x\n",
               __func__, cell_info->doppler_measurement_hz);
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN: {
        struct nas_lac_lte_extended_intra_earfcn_info *cell_info =
            (struct nas_lac_lte_extended_intra_earfcn_info *)(buf + offset);
        logger(MSG_INFO,
               "%s: NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN\n"
               "\t EARFCN: %.8x\n",
               __func__, cell_info->earfcn);
        arfcn = cell_info->earfcn;
      } break;
      case NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN: {
        struct nas_lac_lte_extended_interfrequency_earfcn *cell_info =
            (struct nas_lac_lte_extended_interfrequency_earfcn *)(buf + offset);
        logger(MSG_INFO,
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
            MSG_INFO,
            "%s: NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN\n"
            "\t Number of items: %u\n",
            __func__, cell_info->num_instances);
        for (uint8_t i = 0; i < cell_info->num_instances; i++) {
          logger(MSG_INFO,
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

  process_current_network_data(mcc, mnc, type_of_service, lac, phy_cell_id,
                               cell_id, bsic, bcch, psc, arfcn, srx_lev,
                               rx_lev);
}

void nas_update_network_data(uint8_t network_type, uint8_t signal_level) {
  logger(MSG_DEBUG, "%s: update network data\n", __func__);
  nas_runtime.previous_network_state = nas_runtime.current_network_state;
  nas_runtime.current_network_state.signal_level = 0;
  switch (network_type) {
  case NAS_SIGNAL_REPORT_TYPE_CDMA:     // CDMA
  case NAS_SIGNAL_REPORT_TYPE_CDMA_HDR: // QCOM HDR CDMA
    nas_runtime.current_network_state.network_type = 1;
    break;
  case NAS_SIGNAL_REPORT_TYPE_GSM: // GSM
    nas_runtime.current_network_state.network_type = 4;
    break;
  case NAS_SIGNAL_REPORT_TYPE_WCDMA: // WCDMA
    nas_runtime.current_network_state.network_type = 5;
    break;
  case NAS_SIGNAL_REPORT_TYPE_LTE: // LTE
    nas_runtime.current_network_state.network_type = 8;
    break;
  default:
    logger(MSG_DEBUG, "Unknown signal report: %u\n", network_type);
    nas_runtime.current_network_state.network_type = 0;
    break;
  }

  if (signal_level > 0)
    nas_runtime.current_network_state.signal_level = signal_level / 2;

  if (nas_runtime.current_network_state.network_type <
      nas_runtime.previous_network_state.network_type) {
    uint8_t reply[MAX_MESSAGE_SIZE] = {0};
    size_t strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "NASdbg: Service downgraded!\nService Capability:\n"
        "\tGSM: %s"
        "\tGPRS: %s"
        "\tEDGE: %s\n"
        "\tWCDMA: %s"
        "\tHSDPA: %s"
        "\tHSUPA: %s\n"
        "\tHSDPA+: %s"
        "\tDC-HSDPA+: %s"
        "\tLTE: %s\n"
        "\tCS Attached: %s\n"
        "\tPS attached: %s\n",
        nas_runtime.current_state.service_capability.gsm == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.gprs == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.edge == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.wcdma == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsdpa == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsupa == 1 ? "Yes" : "No",
        nas_runtime.current_state.service_capability.hsdpa_plus == 1 ? "Yes"
                                                                     : "No",
        nas_runtime.current_state.service_capability.dc_hsdpa_plus == 1 ? "Yes"
                                                                        : "No",
        nas_runtime.current_state.service_capability.lte == 1 ? "Yes" : "No",
        nas_runtime.current_state.circuit_switch_attached == 1 ? "Yes" : "No",
        nas_runtime.current_state.packet_switch_attached == 1 ? "Yes" : "No");
    add_message_to_queue(reply, strsz);
  }
}
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_nas_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);
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
    logger(MSG_INFO, "%s: Get Cell Location Info\n", __func__);
    update_cell_location_information(buf, buf_len);
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
    logger(MSG_INFO, "%s: Get Signal Info\n", __func__);
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
  nas_register_to_events();
  nas_get_ims_preference();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}

/*
We need a thread to keep track of these...
get signal info should be triggered every few seconds, and cell location info
too
*/