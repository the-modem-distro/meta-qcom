// SPDX-License-Identifier: MIT

#include "../inc/call.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/proxy.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * Simulated calls
 *  Incoming:
 *      call_service_handler() will catch our number
 *      It should set the flags to start sending QMI
 *      responses to ModemManager, so it gets its'
 *      ATTEMPT, ALERTING and ESTABLISHED states.
 *      FIXME: Make it non-blocking --> TESTING
 *
 *  Outgoing:
 *      First we set the call_pending_flag via set_pending_call_flag()
 *      This allows the rmnet_proxy to know we need to do something
 *      in the next run.
 *
 *      In that next pass, we go through process_simulated_packet()
 *      and we send the first outgoing call notification (0x002e)
 *      On each run, we send a ringing indication to the host
 *      until it picks up
 *
 *      On pick up we send an ESTABLISHED, and wait for a hang up
 *      request while allowing the rest of the traffic to flow
 */

struct {
  uint8_t is_call_pending;
  uint8_t call_simulation_mode; // is in progress?
  uint8_t simulated_call_direction;
  uint8_t simulated_call_state; // ATTEMPT, RINGING, ESTABLISHED...
  uint16_t transaction_id;
} simulated_call_runtime;

void set_call_simulation_mode(bool en) {
  if (en) {
    simulated_call_runtime.call_simulation_mode = 1;
  } else {
    simulated_call_runtime.call_simulation_mode = 0;
  }
}

uint8_t get_call_simulation_mode() {
  return simulated_call_runtime.call_simulation_mode;
}

void reset_call_state() {
  simulated_call_runtime.is_call_pending = false;
  simulated_call_runtime.call_simulation_mode = 0;
  simulated_call_runtime.simulated_call_direction = 0; // NONE
}

void set_pending_call_flag(bool en) {
  if (en) {
    simulated_call_runtime.is_call_pending = true;
  } else {
    simulated_call_runtime.is_call_pending = false;
  }
}

uint8_t get_call_pending() {
  if (simulated_call_runtime.is_call_pending) {
    logger(MSG_INFO, "%s: I need to call you!\n", __func__);
  }
  return simulated_call_runtime.is_call_pending;
}

void send_call_request_response(uint8_t usbfd, uint16_t transaction_id) {
  struct call_request_response_packet *pkt;
  int bytes_written;
  int pkt_size = sizeof(struct call_request_response_packet);
  logger(MSG_WARN, "*** >> Sending Call Request Response\n");
  pkt = calloc(1, sizeof(struct call_request_response_packet));
  /* QMUX */
  pkt->qmuxpkt.version = 0x01;
  pkt->qmuxpkt.packet_length = pkt_size - sizeof(uint8_t); // SIZE
  pkt->qmuxpkt.control = 0x80;
  pkt->qmuxpkt.service = 0x09;
  pkt->qmuxpkt.instance_id = 0x02;
  /*QMI */
  pkt->qmipkt.ctlid = 0x04;
  pkt->qmipkt.transaction_id = transaction_id;
  pkt->qmipkt.msgid = VO_SVC_CALL_REQUEST;
  pkt->qmipkt.length = sizeof(struct call_request_response_packet) -
                       sizeof(struct qmux_packet) -
                       sizeof(struct qmi_packet); // SIZE + 6

  /* QMI Result */
  pkt->indication.result_code_type = 0x02;
  pkt->indication.generic_result_size = 0x04;
  pkt->indication.result = 0x00;
  pkt->indication.response = 0x00;

  /* Call UID */
  pkt->call_id.id = 0x10;
  pkt->call_id.len = 0x01;
  pkt->call_id.data = 0x01;

  /* Media ID */
  pkt->media_id.id = 0x15;
  pkt->media_id.len = 0x01;
  pkt->media_id.data = 0x03; // Dont know what 0x03 means

  logger(MSG_WARN, "Sending request ACK \n");
  dump_pkt_raw((void *)pkt, sizeof(struct call_request_response_packet));

  bytes_written = write(usbfd, pkt, pkt_size);
  free(pkt);
  pkt = NULL;
}

uint8_t send_voice_call_status_event(uint8_t usbfd, uint16_t transaction_id,
                                     uint8_t call_direction,
                                     uint8_t call_state) {
  logger(MSG_WARN, "%s: Send QMI call status message: Direction %i, state %i\n",
         __func__, call_direction, call_state);
  uint8_t phone_proto[] = {0x2b, 0x30, 0x31, 0x35, 0x35, 0x35, 0x30,
                           0x31, 0x39, 0x39, 0x39, 0x39, 0x39};
  struct simulated_call_packet *pkt;
  int bytes_written;
  pkt = calloc(1, sizeof(struct simulated_call_packet));
  int pkt_size = sizeof(struct simulated_call_packet);
  /* QMUX */
  pkt->qmuxpkt.version = 0x01;
  pkt->qmuxpkt.packet_length = pkt_size - sizeof(uint8_t);
  pkt->qmuxpkt.control = 0x80;
  pkt->qmuxpkt.service = 0x09;
  pkt->qmuxpkt.instance_id = 0x02;

  /*QMI */
  pkt->qmipkt.ctlid = 0x04;
  pkt->qmipkt.transaction_id = transaction_id;
  pkt->qmipkt.msgid = VO_SVC_CALL_STATUS;
  pkt->qmipkt.length =
      pkt_size - sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  /* META */
  pkt->meta.tlvid = 0x01;
  pkt->meta.length = 0x08;
  pkt->meta.unk1 = 0x01;
  pkt->meta.unk2 = 0x01;
  pkt->meta.call_state = call_state;
  pkt->meta.unk4 = 0x02;
  pkt->meta.call_direction = call_direction;
  pkt->meta.call_type = CALL_TYPE_VOLTE;
  pkt->meta.unk7 = 0x00;
  pkt->meta.unk8 = 0x00;

  /* REMOTE PARTY NUMBER */
  pkt->number.tlvid = 0x10;
  pkt->number.len = 0x11;
  pkt->number.data[0] = 0x01;
  pkt->number.data[1] = 0x01;
  pkt->number.data[2] = 0x00;
  pkt->number.phone_num_length = 0x0d;
  memcpy(pkt->number.phone_number, phone_proto, 0x0d);

  pkt->info.id = 0x027;
  pkt->info.len = 0x03;
  pkt->info.data[0] = 0x01;
  pkt->info.data[1] = 0x01;
  pkt->info.data[2] = 0x02; // 0x03? 0x01? This moves...

  pkt->remote_party_extension.id = 0x2b;
  pkt->remote_party_extension.len = 0x14;
  // 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 || 0x01,
  pkt->remote_party_extension.data[0] = 0x01;
  pkt->remote_party_extension.data[1] = 0x01;
  pkt->remote_party_extension.data[2] = 0x00;
  pkt->remote_party_extension.data[3] = 0x00;
  pkt->remote_party_extension.data[4] = 0x00;
  // IS INTERNATIONAL NUMBER || 0x00 IS LOCAL
  pkt->remote_party_extension.data[5] = 0x01;
  pkt->remote_party_extension.phone_number_size = 0x0d;
  memcpy(pkt->remote_party_extension.phone_number, phone_proto,
         sizeof(phone_proto));

  dump_pkt_raw((void *)pkt, sizeof(struct simulated_call_packet));
  bytes_written = write(usbfd, pkt, pkt_size);

  free(pkt);
  pkt = NULL;
  return 0;
}

void accept_simulated_call(uint8_t usbfd, uint8_t transaction_id) {
  struct call_accept_ack *pkt;
  logger(MSG_WARN, "%s: Send QMI call ACCEPT message\n", __func__);
  int bytes_written;
  pkt = calloc(1, sizeof(struct call_accept_ack));
  int pkt_size = sizeof(struct call_accept_ack);

  /* QMUX */
  pkt->qmuxpkt.version = 0x01;
  pkt->qmuxpkt.packet_length = pkt_size - sizeof(uint8_t);
  pkt->qmuxpkt.control = 0x80;
  pkt->qmuxpkt.service = 0x09;
  pkt->qmuxpkt.instance_id = 0x02;

  /*QMI */
  pkt->qmipkt.ctlid = 0x02;
  pkt->qmipkt.transaction_id = transaction_id;
  pkt->qmipkt.msgid = VO_SVC_CALL_ANSWER_REQ;
  pkt->qmipkt.length =
      pkt_size - sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  /* RESULT INDICATION */
  pkt->indication.result_code_type = 0x02;
  pkt->indication.generic_result_size = 0x04;
  pkt->indication.result = 0x00;
  pkt->indication.response = 0x00;

  /* CALL ID */
  pkt->call_id.id = 0x10;
  pkt->call_id.len = 0x01;
  pkt->call_id.data = 0x01;

  dump_pkt_raw((void *)pkt, sizeof(struct call_accept_ack));
  bytes_written = write(usbfd, pkt, pkt_size);

  logger(MSG_WARN, "%s: SEND PKT: %i bytes written \n", __func__,
         bytes_written);
  free(pkt);
  pkt = NULL;
}

void start_simulated_call(uint8_t usbfd) {
  simulated_call_runtime.transaction_id = 0x01;
  simulated_call_runtime.simulated_call_state = AUDIO_CALL_RINGING;
  simulated_call_runtime.simulated_call_direction = AUDIO_DIRECTION_INCOMING;
  set_call_simulation_mode(true);
  logger(MSG_WARN, " Call update notification: RINGING \n");
  send_voice_call_status_event(usbfd, simulated_call_runtime.transaction_id,
                               AUDIO_DIRECTION_INCOMING, AUDIO_CALL_RINGING);
}

uint8_t send_voice_cal_disconnect_ack(uint8_t usbfd, uint16_t transaction_id) {
  struct end_call_response *pkt;
  logger(MSG_WARN, "%s: Send QMI call END message\n", __func__);
  int bytes_written;
  pkt = calloc(1, sizeof(struct end_call_response));
  int pkt_size = sizeof(struct end_call_response);

  /* QMUX */
  pkt->qmuxpkt.version = 0x01;
  pkt->qmuxpkt.packet_length = pkt_size - sizeof(uint8_t);
  pkt->qmuxpkt.control = 0x80;
  pkt->qmuxpkt.service = 0x09;
  pkt->qmuxpkt.instance_id = 0x02;

  /*QMI */
  pkt->qmipkt.ctlid = 0x02;
  pkt->qmipkt.transaction_id = transaction_id;
  pkt->qmipkt.msgid = VO_SVC_CALL_STATUS;
  pkt->qmipkt.length =
      pkt_size - sizeof(struct qmux_packet) - sizeof(struct qmi_packet);

  /* RESULT INDICATION */
  pkt->indication.result_code_type = 0x02;
  pkt->indication.generic_result_size = 0x04;
  pkt->indication.result = 0x00;
  pkt->indication.response = 0x00;

  /* CALL ID */
  pkt->call_id.id = 0x10;
  pkt->call_id.len = 0x01;
  pkt->call_id.data = 0x01;

  dump_pkt_raw((void *)pkt, sizeof(struct end_call_response));
  bytes_written = write(usbfd, pkt, pkt_size);

  logger(MSG_WARN, "%s: SEND PKT: %i bytes written \n", __func__,
         bytes_written);
  free(pkt);
  pkt = NULL;
  return 0;
}

void process_incoming_call_accept(uint8_t usbfd, uint16_t transaction_id) {
  pthread_t dummy_echo_thread;
  int ret;
  logger(MSG_WARN, "[SEND ACCEPT CALL ACK] \n");
  accept_simulated_call(usbfd, transaction_id);
  usleep(100000);
  logger(MSG_WARN, "%s: [ESTABLISHING CALL]\n", __func__);
  send_voice_call_status_event(usbfd, transaction_id + 1,
                               AUDIO_DIRECTION_INCOMING,
                               AUDIO_CALL_ESTABLISHED);
  usleep(100000);
  if ((ret =
           pthread_create(&dummy_echo_thread, NULL, &can_you_hear_me, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }
}
void send_dummy_call_established(uint8_t usbfd, uint16_t transaction_id) {
  pthread_t dummy_echo_thread;
  int ret;
  logger(MSG_WARN, "Call update notification 1 \n");
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ATTEMPT);
  usleep(100000);
  logger(MSG_WARN, "Call request response \n");
  send_call_request_response(usbfd, transaction_id);
  usleep(100000);
  logger(MSG_WARN, " Call update notification 2: ALERTING \n");
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ALERTING);
  usleep(100000);
  logger(MSG_WARN, "Call established notification \n");
  transaction_id++;
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ESTABLISHED);
  usleep(100000);
  if ((ret =
           pthread_create(&dummy_echo_thread, NULL, &can_you_hear_me, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }
}
uint8_t call_service_handler(uint8_t source, void *bytes, size_t len,
                             uint16_t msgid, uint8_t adspfd, uint8_t usbfd) {
  int needs_rerouting = 0, i, j;
  struct encapsulated_qmi_packet *pkt;
  struct call_request_indication *call_request;
  struct call_status_indication *call_indication;

  pkt = (struct encapsulated_qmi_packet *)bytes;
  // 015550199999 in ASCII
  uint8_t our_phone[] = {0x30, 0x31, 0x35, 0x35, 0x35, 0x30,
                         0x31, 0x39, 0x39, 0x39, 0x39, 0x39};
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
  memset(phone_number, 0, MAX_PHONE_NUMBER_LENGTH);
  if (source == FROM_HOST) {
    logger(MSG_INFO, "%s [tracking] Voice packet from host\n", __func__);
  } else if (source == FROM_DSP) {
    logger(MSG_INFO, "%s [tracking] Voice packet from DSP\n", __func__);
  } else {
    logger(MSG_INFO, "%s [tracking] Voice packet [UNKNOWN_SOURCE]\n", __func__);
  }
  dump_pkt_raw(bytes, len);
  switch (msgid) {
  case VO_SVC_CALL_REQUEST:
    j = 0;
    call_request = (struct call_request_indication *)bytes;
    logger(MSG_WARN, "%s: Call request to %s\n", __func__,
           call_request->number.phone_number);
    for (i = 0; i < call_request->number.phone_num_length; i++) {
      if (call_request->number.phone_number[i] >= 0x30 &&
          call_request->number.phone_number[i] <= 0x39) {
        phone_number[j] = call_request->number.phone_number[i];
        j++;
      }
    }
    if (j > 0) {
      if (memcmp(phone_number, our_phone, 12) == 0) {
        logger(MSG_WARN, "%s: Call BYPASS: %s\n", __func__, phone_number);
        set_call_simulation_mode(true);
        send_dummy_call_established(usbfd, call_request->qmipkt.transaction_id);
        simulated_call_runtime.simulated_call_direction =
            AUDIO_DIRECTION_OUTGOING;
        needs_rerouting = 1;
      } else {
        logger(MSG_WARN, "%s: Call PASS THROUGH: %s\n", __func__, phone_number);

        set_call_simulation_mode(false);
        simulated_call_runtime.simulated_call_direction = 0;
      }
    }
    break;
  case VO_SVC_CALL_ANSWER_REQ:
    if (get_call_simulation_mode()) {
      logger(MSG_WARN, "%s: Admin wants to talk!\n", __func__);
      process_incoming_call_accept(usbfd, pkt->qmi.transaction_id);
      needs_rerouting = 1;
    }
    break;
  case VO_SVC_CALL_INFO:
    logger(MSG_WARN, "%s: CALL INFO\n", __func__);
    break;
  case VO_SVC_CALL_STATUS:
    call_indication = (struct call_status_indication *)bytes;
    j = 0;
    for (i = 0; i < call_indication->number.phone_num_length; i++) {
      if (call_indication->number.phone_number[i] >= 0x30 &&
          call_indication->number.phone_number[i] <= 0x39) {
        phone_number[j] = call_indication->number.phone_number[i];
        j++;
      }
    }
    /* Caller ID is set */
    if (j > 0) {
      logger(MSG_WARN, "%s: Call status: %s\n", __func__, phone_number);
      if (memcmp(phone_number, our_phone, 12) == 0) {
        logger(MSG_WARN, "%s: Call BYPASS: %s\n", __func__, phone_number);
        needs_rerouting = 1;
      } else {
        handle_call_pkt(bytes, len, phone_number);
      }
    } else {
      /* Caller ID is *NOT* set */
      if (get_call_simulation_mode()) {
        needs_rerouting = 1;
      } else {
        logger(MSG_WARN, "%s: Unknown number %s\n", __func__, phone_number);
        memcpy((uint8_t *)phone_number, (uint8_t *)"Unknown",
               strlen("Unknown"));
        handle_call_pkt(bytes, len, phone_number);
      }
    }

    break;
  case VO_SVC_CALL_STATUS_CHANGE:
    logger(MSG_WARN, "%s: State change in call.. on hold?\n", __func__);
    break;
  case VO_SVC_CALL_END_REQ:
    logger(MSG_WARN, "%s: Request to terminate the call\n", __func__);
    if (get_call_simulation_mode()) {
      logger(MSG_WARN, "%s: Why you hang up on me!!\n", __func__);
      send_voice_cal_disconnect_ack(usbfd, pkt->qmi.transaction_id);
      usleep(100000);
      logger(MSG_WARN, "%s: [DISCONNECTING]\n", __func__);
      send_voice_call_status_event(
          usbfd, pkt->qmi.transaction_id,
          simulated_call_runtime.simulated_call_direction,
          AUDIO_CALL_DISCONNECTING);
      set_call_simulation_mode(false);
      simulated_call_runtime.simulated_call_direction = 0;
      needs_rerouting = 1;
    }
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled packet: 0x%.4x\n", __func__, msgid);
    break;
  }

  call_indication = NULL;
  call_request = NULL;
  pkt = NULL;
  return needs_rerouting;
}