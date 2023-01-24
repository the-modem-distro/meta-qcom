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

const char *get_nas_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(nas_svc_commands) / sizeof(nas_svc_commands[0])); i++) {
    if (nas_svc_commands[i].id == msgid) {
      return nas_svc_commands[i].cmd;
    }
  }
  return "Voice: Unknown command\n";
}

int nas_register_to_events() {
  logger(MSG_INFO, "********* NAS START\n");
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (19 * sizeof(struct qmi_generic_uint8_t_tlv));
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

/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_nas_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);

  switch (get_qmi_message_id(buf, buf_len)) {
  default:
    logger(MSG_INFO, "%s: Unhandled message for the NAS Service: %.4x\n",
           __func__, get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}

void *register_to_nas_service() {
  nas_register_to_events();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}