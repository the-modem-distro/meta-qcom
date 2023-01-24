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
#include "../inc/nas.h"
#include "../inc/qmi.h"


struct {
  uint8_t operator_name[32];
  uint8_t mcc[3];
  uint8_t mnc[2];
  uint16_t location_area_code_1;
  uint16_t location_area_code_2;
  struct service_capability current_service_capability;
  uint8_t network_registration_status;
  uint8_t circuit_switch_attached;
  uint8_t packet_switch_attached;
  uint8_t radio_access;
} nas_runtime;

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
  logger(MSG_INFO, "********* NAS START\n");
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
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_VOICE, 0) < 0) {
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
  logger(MSG_INFO, "********* NAS ADD REQUEST\n");

  add_pending_message(QMI_SERVICE_NAS, (uint8_t *)pkt, pkt_len);
  logger(MSG_INFO, "********* NAS END\n");

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
    memcpy(nas_runtime.operator_name, name->operator_name,
           name->len - (sizeof(uint16_t)));
  }
  offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
  if (offset > 0) {
    struct carrier_mcc_mnc *op_code = (struct carrier_mcc_mnc *)(buf + offset);
    if (buf_len < op_code->len) {
      logger(MSG_ERROR, "%s: Message is shorter than carrier data\n", __func__);
      return;
    }
    memcpy(nas_runtime.mcc, op_code->mcc, 3);
    memcpy(nas_runtime.mnc, op_code->mnc, 2);
    nas_runtime.location_area_code_1 = op_code->lac1;
    nas_runtime.location_area_code_2 = op_code->lac2;
  }
}

void parse_serving_system_message(uint8_t *buf, size_t buf_len) {
  /* There can be a lot of tlvs here, but we care about 2, service and capability*/
  int offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
  if (offset > 0) {
    struct empty_tlv *capability_arr =
        (struct empty_tlv *)(buf + offset);
    if (buf_len < capability_arr->len) {
      logger(MSG_ERROR, "%s: Message is shorter than the operator name\n",
             __func__);
      return;
    }
    // Set everything to 0, then repopulate
    nas_runtime.current_service_capability.gprs = 0;
    nas_runtime.current_service_capability.edge = 0;
    nas_runtime.current_service_capability.hsdpa = 0;
    nas_runtime.current_service_capability.hsupa = 0;
    nas_runtime.current_service_capability.wcdma = 0;
    nas_runtime.current_service_capability.gsm = 0;
    nas_runtime.current_service_capability.lte = 0;
    nas_runtime.current_service_capability.hsdpa_plus = 0;
    nas_runtime.current_service_capability.dc_hsdpa_plus = 0;

    for (uint16_t i = 0; i < capability_arr->len; i++) {
        switch (capability_arr->data[i]) {
            case 0x01:
            nas_runtime.current_service_capability.gprs = 1;
            break;
            case 0x02:
            nas_runtime.current_service_capability.edge = 1;
            break;
            case 0x03:
            nas_runtime.current_service_capability.hsdpa = 1;
            break;
            case 0x04:
            nas_runtime.current_service_capability.hsupa = 1;
            break;
            case 0x05:
            nas_runtime.current_service_capability.wcdma = 1;
            break;
            case 0x0a:
            nas_runtime.current_service_capability.gsm = 1;
            break;
            case 0x0b:
            nas_runtime.current_service_capability.lte = 1;
            break;
            case 0x0c:
            nas_runtime.current_service_capability.hsdpa_plus = 1;
            break;
            case 0x0d:
            nas_runtime.current_service_capability.dc_hsdpa_plus = 1;
            break;
            default:
                logger(MSG_WARN, "%s: Unknown service capability: %.2x\n", __func__, capability_arr->data[i]);
                break;
        }
    }
  }
  offset = get_tlv_offset_by_id(buf, buf_len, 0x01);
  if (offset > 0) {
    struct nas_serving_system_state *serving_sys = (struct nas_serving_system_state *)(buf + offset);
    if (buf_len < serving_sys->len) {
      logger(MSG_ERROR, "%s: Message is shorter than carrier data\n", __func__);
      return;
    }
    nas_runtime.network_registration_status = serving_sys->registration_status;
    nas_runtime.circuit_switch_attached = serving_sys->cs_attached;
    nas_runtime.packet_switch_attached = serving_sys->ps_attached;
    nas_runtime.radio_access = serving_sys->radio_access;
  }
}
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_nas_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);

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
  case NAS_RESET:
    logger(MSG_INFO, "%s: Reset\n", __func__);
    break;
  case NAS_ABORT:
    logger(MSG_INFO, "%s: Abort\n", __func__);
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

  logger(MSG_INFO,
         "%s: Report\n"
         "\tOperator: %s\n"
         "\t MCC-MNC: %s-%s\n"
         "\t LACs: %.4x %.4x\n",
         __func__, nas_runtime.operator_name, nas_runtime.mcc, nas_runtime.mnc,
         nas_runtime.location_area_code_1, nas_runtime.location_area_code_2);

    logger(MSG_INFO, "%s: Capabilities\n"
                     "\tGSM: %s\n"
                     "\tGPRS: %s\n"
                     "\tEDGE: %s\n"
                     "\tWCDMA: %s\n"
                     "\tHSDPA: %s\n"
                     "\tHSUPA: %s\n"
                     "\tHSDPA+: %s\n"
                     "\tDC-HSDPA+: %s\n"
                     "\tLTE: %s\n"
                     "\tCircuit Switch Service Attached: %s\n"
                     "\tPacket Switch Service attached: %s\n",
                     __func__,
                    nas_runtime.current_service_capability.gsm == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.gprs == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.edge == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.wcdma == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.hsdpa == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.hsupa == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.hsdpa_plus == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.dc_hsdpa_plus == 1 ? "Yes" : "No",
                    nas_runtime.current_service_capability.lte == 1 ? "Yes" : "No",
                    nas_runtime.circuit_switch_attached == 1 ? "Yes" : "No",
                    nas_runtime.packet_switch_attached == 1 ? "Yes" : "No"
    );

  return 0;
}// source == FROM_HOST ? "HOST" : "ADSP"

void *register_to_nas_service() {
  nas_register_to_events();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}