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
#include "../inc/voice.h"

const char *get_voice_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(voice_svc_commands) / sizeof(voice_svc_commands[0])); i++) {
    if (voice_svc_commands[i].id == msgid) {
      return voice_svc_commands[i].cmd;
    }
  }
  return "Voice: Unknown command\n";
}


int voice_register_to_events() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (19 * sizeof(struct qmi_generic_uint8_t_tlv));
  uint8_t *pkt = malloc(pkt_len);
  uint8_t tlvs[] = {
      VOICE_EVENT_DTMF,
      VOICE_EVENT_VOICE_PRIVACY,
      VOICE_EVENT_SUPPLEMENTARY_SERVICES,
      VOICE_EVENT_CALL_EVENTS,
      VOICE_EVENT_HANDOVER_EVENTS,
      VOICE_EVENT_SPEECH_EVENTS,
      VOICE_EVENT_USSD_NOTIFICATIONS,
      VOICE_EVENT_MODIFICATION_EVENTS,
      VOICE_EVENT_UUS_EVENTS,
      VOICE_EVENT_AOC_EVENTS,
      VOICE_EVENT_CONFERENCE_EVENTS,
      VOICE_EVENT_CALL_CONTROL_EVENTS,
      VOICE_EVENT_CONFERENCE_PARTICIPANT_EVENT,
      VOICE_EVENT_EMERG_CALL_ORIGINATION_FAILURE_EVENTS,
      VOICE_EVENT_ADDITIONAL_CALL_INFO_EVENTS,
      VOICE_EVENT_EMERGENCY_CALL_STATUS_EVENTS,
      VOICE_EVENT_CALL_REESTABLISHMENT_STATUS,
      VOICE_EVENT_EMERGENCY_CALL_OPERATING_MODE_EVENT,
      VOICE_EVENT_AUTOREJECTED_INCOMING_CALL_END_EVENT};
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_VOICE, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, VOICE_INDICATION_REGISTER) <
      0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  for (int i = 0; i < 19; i++) {

    if (build_u8_tlv(pkt, pkt_len, curr_offset,
                     tlvs[i], 1) < 0) {
      logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  }

  add_pending_message(QMI_SERVICE_VOICE, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}

/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_voice_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);

  switch (get_qmi_message_id(buf, buf_len)) {
  default:
    logger(MSG_INFO, "%s: Unhandled message for the Voice Service: %.4x\n",
           __func__, get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}

void *register_to_voice_service() {
  voice_register_to_events();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}