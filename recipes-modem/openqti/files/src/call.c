// SPDX-License-Identifier: MIT

#include "../inc/call.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/command.h"
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

#define MAX_TTS_TEXT_SIZE 160 // For now just like SMS
struct message {
  char message[MAX_TTS_TEXT_SIZE]; // JUST TEXT
  int len;                         // TEXT SIZE
  uint8_t state;                   // message sending status 0 empty, 1 pending
};

struct {
  uint8_t is_call_pending;
  uint8_t call_simulation_mode; // is in progress?
  uint8_t sim_call_direction;
  uint8_t simulated_call_state; // ATTEMPT, RINGING, ESTABLISHED...
  uint16_t transaction_id;
  uint8_t empty_message_loop;
  uint8_t timeout_counter;
  bool stick_to_looped_message;
  struct message msg[QUEUE_SIZE];
} call_rt;

void set_call_simulation_mode(bool en) {
  if (en) {
    call_rt.call_simulation_mode = 1;
  } else {
    call_rt.call_simulation_mode = 0;
  }
}
void set_looped_message(bool en) {
  if (en) {
    call_rt.stick_to_looped_message = true;
  } else {
    call_rt.stick_to_looped_message = false;
  }
}

uint8_t get_call_simulation_mode() { return call_rt.call_simulation_mode; }

void reset_call_state() {
  call_rt.is_call_pending = false;
  call_rt.call_simulation_mode = 0;
  call_rt.empty_message_loop = 0;
  call_rt.timeout_counter = 0;
  call_rt.stick_to_looped_message = false;
  call_rt.sim_call_direction = 0; // NONE
  for (int i = 0; i < QUEUE_SIZE; i++) {
    call_rt.msg[i].state = 0;
    call_rt.msg[i].len = 0;
    memset(call_rt.msg[i].message, 0, MAX_TTS_TEXT_SIZE);
  }
}

void set_pending_call_flag(bool en) {
  if (en && !call_rt.call_simulation_mode) {
    call_rt.is_call_pending = true;
  } else {
    call_rt.is_call_pending = false;
  }
}

uint8_t get_call_pending() {
  if (call_rt.is_call_pending) {
    logger(MSG_INFO, "%s: I need to call you!\n", __func__);
  }
  return call_rt.is_call_pending;
}

void send_call_request_response(int usbfd, uint16_t transaction_id) {
  struct call_request_response_packet *pkt;
  int bytes_written;
  int pkt_size = sizeof(struct call_request_response_packet);
  logger(MSG_INFO, "%s: Sending Call Request Response\n", __func__);
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

  bytes_written = write(usbfd, pkt, pkt_size);
  dump_pkt_raw((void *)pkt, sizeof(struct call_request_response_packet));
  logger(MSG_DEBUG, "%s: Sent %i bytes \n", __func__, bytes_written);

  free(pkt);
  pkt = NULL;
}

uint8_t send_voice_call_status_event(int usbfd, uint16_t transaction_id,
                                     uint8_t call_direction,
                                     uint8_t call_state) {
  logger(MSG_INFO, "%s: Send QMI call status message: Direction %i, state %i\n",
         __func__, call_direction, call_state);
  uint8_t phone_proto[] = {0x2b, 0x30, 0x31, 0x35, 0x35, 0x35, 0x30,
                           0x31, 0x39, 0x39, 0x39, 0x39, 0x39};
  struct simulated_call_packet *pkt;
  int bytes_written;
  pkt = calloc(1, sizeof(struct simulated_call_packet));
  int pkt_size = sizeof(struct simulated_call_packet);

  // Update last transaction_id
  call_rt.transaction_id = transaction_id;
  call_rt.sim_call_direction = call_direction;
  call_rt.simulated_call_state = call_state;
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

  bytes_written = write(usbfd, pkt, pkt_size);
  dump_pkt_raw((void *)pkt, sizeof(struct simulated_call_packet));
  logger(MSG_DEBUG, "%s: Sent %i bytes \n", __func__, bytes_written);
  free(pkt);
  pkt = NULL;
  return 0;
}

void accept_simulated_call(int usbfd, uint8_t transaction_id) {
  struct call_accept_ack *pkt;
  logger(MSG_INFO, "%s: Send QMI call ACCEPT message\n", __func__);
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

  bytes_written = write(usbfd, pkt, pkt_size);
  dump_pkt_raw((void *)pkt, sizeof(struct call_accept_ack));
  logger(MSG_DEBUG, "%s: Sent %i bytes \n", __func__, bytes_written);

  free(pkt);
  pkt = NULL;
}

void close_internal_call(int usbfd, uint16_t transaction_id) {
  send_voice_call_status_event(usbfd, transaction_id,
                               call_rt.sim_call_direction,
                               AUDIO_CALL_DISCONNECTING);
  usleep(100000);
  send_voice_call_status_event(usbfd, transaction_id + 1,
                               call_rt.sim_call_direction, AUDIO_CALL_HANGUP);

  set_call_simulation_mode(false);
  call_rt.sim_call_direction = 0;
}

void start_simulated_call(int usbfd) {
  pthread_t kill_timeout_thread;
  int ret;
  call_rt.transaction_id = 0x01;
  call_rt.simulated_call_state = AUDIO_CALL_RINGING;
  call_rt.sim_call_direction = AUDIO_DIRECTION_INCOMING;
  set_call_simulation_mode(true);
  pulse_ring_in();
  usleep(200000);
  logger(MSG_WARN, " Call update notification: RINGING to %x\n", usbfd);
  send_voice_call_status_event(usbfd, call_rt.transaction_id,
                               AUDIO_DIRECTION_INCOMING, AUDIO_CALL_RINGING);
}

uint8_t send_voice_cal_disconnect_ack(int usbfd, uint16_t transaction_id) {
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

  bytes_written = write(usbfd, pkt, pkt_size);
  logger(MSG_DEBUG, "%s: Sent %i bytes \n", __func__, bytes_written);
  dump_pkt_raw((void *)pkt, sizeof(struct end_call_response));
  free(pkt);
  pkt = NULL;
  return 0;
}

void notify_simulated_call(int usbfd) {
  if (get_call_simulation_mode() &&
      call_rt.empty_message_loop > CALL_MAX_LOOPS) {
    // Automatically close the call
    logger(MSG_INFO, "%s: Automatically closing the call\n", __func__);
    close_internal_call(usbfd, call_rt.transaction_id);
    reset_call_state();
  }

  if (call_rt.simulated_call_state != AUDIO_CALL_ESTABLISHED) {
    if (call_rt.timeout_counter > 60) {
      logger(MSG_WARN, "%s: Killing internal call\n", __func__);
      close_internal_call(usbfd, call_rt.transaction_id);
      reset_call_state();
    } else {
      call_rt.timeout_counter++;
    }
  }
}

void add_voice_message_to_queue(uint8_t *message, size_t len) {
  int i;
  if (len > 0) {
    for (i = 0; i < QUEUE_SIZE; i++) {
      if (call_rt.msg[i].state == 0) {
        memset(call_rt.msg[i].message, 0, MAX_TTS_TEXT_SIZE);
        memcpy(call_rt.msg[i].message, message, len);
        call_rt.msg[i].len = len;
        call_rt.msg[i].state = 1;
        return;
      }
    }
  }
}

void *simulated_call_tts_handler() {
  char *buffer;
  int size;
  int num_read;
  FILE *file;
  struct pcm *pcm0;
  int i;
  bool handled;
  char *phrase; //[MAX_TTS_TEXT_SIZE];
  /*
   * Open PCM if we're in call simulation mode,
   * Then leave it open until we're finished
   */

  if (get_call_simulation_mode()) {
    logger(MSG_INFO, "%s: Started TTS thread\n", __func__);
    /* Initial set up of the audio codec */

    set_multimedia_mixer();

    pcm0 = pcm_open((PCM_OUT | PCM_MONO), PCM_DEV_HIFI);
    if (pcm0 == NULL) {
      logger(MSG_INFO, "%s: Error opening %s, custom alert tone won't play\n",
             __func__, PCM_DEV_HIFI);
      return NULL;
    }

    pcm0->channels = 1;
    pcm0->flags = PCM_OUT | PCM_MONO;
    pcm0->format = PCM_FORMAT_S16_LE;
    pcm0->rate = 16000;
    pcm0->period_size = 1024;
    pcm0->period_cnt = 1;
    pcm0->buffer_size = 32768;
    if (set_params(pcm0, PCM_OUT)) {
      logger(MSG_ERROR, "Error setting TX Params\n");
      pcm_close(pcm0);
      return NULL;
    }

    if (!pcm0) {
      logger(MSG_ERROR, "%s: Unable to open PCM device\n", __func__);
      return NULL;
    }
  }

  while (get_call_simulation_mode()) {
    handled = false;
    phrase = malloc(MAX_TTS_TEXT_SIZE * sizeof(char));
    for (i = 0; i < QUEUE_SIZE; i++) {
      if (call_rt.msg[i].state == 1 && call_rt.msg[i].len > 0) {
        snprintf(phrase, MAX_TTS_TEXT_SIZE, "%s", call_rt.msg[i].message);
        if (!call_rt.stick_to_looped_message) {
          call_rt.msg[i].state = 0;
        }
        handled = true;
        call_rt.empty_message_loop = 0;
        break;
      }
    }
    if (!handled) {
      logger(MSG_INFO, "%s: Waiting for a command (%i of %i)\n", __func__,
             call_rt.empty_message_loop, CALL_MAX_LOOPS);
      snprintf(phrase, MAX_TTS_TEXT_SIZE,
               "Hello %s. Send me a message an I will answer with voice",
               get_rt_user_name());
      call_rt.empty_message_loop++;
    }
    pico2aud(phrase, strlen(phrase));
    free(phrase);
    phrase = NULL;
    file = fopen("/tmp/wave.wav", "r");
    if (file == NULL) {
      logger(MSG_ERROR, "%s: Unable to open file\n", __func__);
      pcm_close(pcm0);
      return NULL;
    }

    fseek(file, 44, SEEK_SET);
    size = pcm_frames_to_bytes(pcm0, pcm_get_buffer_size(pcm0));
    buffer = malloc(size * 1024);

    if (!buffer) {
      logger(MSG_ERROR, "Unable to allocate %d bytes\n", size);
      free(buffer);
      buffer = NULL;
      return NULL;
    }

    do {
      num_read = fread(buffer, 1, size, file);

      if (num_read > 0) {
        if (pcm_write(pcm0, buffer, num_read)) {
          logger(MSG_ERROR, "Error playing sample\n");
          break;
        }
      }
    } while (num_read > 0 && get_call_simulation_mode());
    fclose(file);
  }

  logger(MSG_INFO, "%s: Cleaning up\n", __func__);

  free(buffer);
  buffer = NULL;
  pcm_close(pcm0);
  stop_multimedia_mixer();

  for (i = 0; i < QUEUE_SIZE; i++) {
    call_rt.msg[i].state = 0;
  }
  return NULL;
}

void process_incoming_call_accept(int usbfd, uint16_t transaction_id) {
  pthread_t dummy_echo_thread;
  int ret;
  logger(MSG_INFO, "%s: Accepting simulated call \n", __func__);
  accept_simulated_call(usbfd, transaction_id);
  usleep(100000);
  logger(MSG_INFO, "%s: Establishing call... \n", __func__);
  send_voice_call_status_event(usbfd, transaction_id + 1,
                               AUDIO_DIRECTION_INCOMING,
                               AUDIO_CALL_ESTABLISHED);
  usleep(100000);
  if ((ret = pthread_create(&dummy_echo_thread, NULL,
                            &simulated_call_tts_handler, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }
}

void send_dummy_call_established(int usbfd, uint16_t transaction_id) {
  pthread_t dummy_echo_thread;
  int ret;
  logger(MSG_INFO, "%s: Sending response: ATTEMPT\n", __func__);
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ATTEMPT);
  usleep(100000);
  logger(MSG_INFO, "%s: Sending response: Call request ACK \n", __func__);
  send_call_request_response(usbfd, transaction_id);
  usleep(100000);
  logger(MSG_INFO, "%s: Sending response: ALERTING\n", __func__);
  transaction_id++;
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ALERTING);
  usleep(100000);
  logger(MSG_INFO, "%s: Sending response: ESTABLISHED\n", __func__);
  transaction_id++;
  send_voice_call_status_event(usbfd, transaction_id, AUDIO_DIRECTION_OUTGOING,
                               AUDIO_CALL_ESTABLISHED);
  usleep(100000);
  if ((ret = pthread_create(&dummy_echo_thread, NULL,
                            &simulated_call_tts_handler, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }

  call_rt.transaction_id = transaction_id; // Save the current transaction ID
}
uint8_t call_service_handler(uint8_t source, void *bytes, size_t len,
                             uint16_t msgid, int adspfd, int usbfd) {
  int needs_rerouting = 0, i, j;
  struct encapsulated_qmi_packet *pkt;
  struct call_request_indication *req;
  struct call_status_indication *ind;

  pkt = (struct encapsulated_qmi_packet *)bytes;
  // 015550199999 in ASCII
  uint8_t our_phone[] = {0x30, 0x31, 0x35, 0x35, 0x35, 0x30,
                         0x31, 0x39, 0x39, 0x39, 0x39, 0x39};
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH];
  memset(phone_number, 0, MAX_PHONE_NUMBER_LENGTH);
  switch (msgid) {

  case VO_SVC_CALL_REQUEST:
    j = 0;
    req = (struct call_request_indication *)bytes;
    for (i = 0; i < req->number.phone_num_length; i++) {
      if (req->number.phone_number[i] >= 0x30 &&
          req->number.phone_number[i] <= 0x39) {
        phone_number[j] = req->number.phone_number[i];
        j++;
      }
    }
    if (j > 0 && memcmp(phone_number, our_phone, 12) == 0) {
      logger(MSG_INFO, "%s: This call is for me: %s\n", __func__, phone_number);
      if (!get_call_simulation_mode()) {
        logger(MSG_INFO, "%s: We're accepting this call\n", __func__);
        set_call_simulation_mode(true);
        send_dummy_call_established(usbfd, req->qmipkt.transaction_id);
        call_rt.sim_call_direction = AUDIO_DIRECTION_OUTGOING;
      } else {
        logger(MSG_ERROR, "%s: We're already talking!\n", __func__);
      }
      needs_rerouting = 1;
    } else {
      logger(MSG_INFO, "%s: This call is for someone else: %s\n", __func__,
             phone_number);
      if (get_call_simulation_mode()) {
        logger(MSG_WARN, "%s: User was talking to us, closing that call\n",
               __func__);
        close_internal_call(usbfd, pkt->qmi.transaction_id);
      }
    }
    req = NULL;
    break;

  case VO_SVC_CALL_ANSWER_REQ:
    if (get_call_simulation_mode()) {
      logger(MSG_WARN, "%s: Admin wants to talk!\n", __func__);
      process_incoming_call_accept(usbfd, pkt->qmi.transaction_id);
      needs_rerouting = 1;
    }
    break;

  case VO_SVC_CALL_INFO: /* FIXME: IMPLEMENT THIS */
    logger(MSG_INFO, "%s: VO_SVC_CALL_INFO: Unimplemented!\n", __func__);
    set_log_level(0);
    dump_pkt_raw(bytes, len);
    break;

  case VO_SVC_CALL_STATUS:
    ind = (struct call_status_indication *)bytes;
    j = 0;
    for (i = 0; i < ind->number.phone_num_length; i++) {
      if (ind->number.phone_number[i] >= 0x30 &&
          ind->number.phone_number[i] <= 0x39) {
        phone_number[j] = ind->number.phone_number[i];
        j++;
      }
    }

    /* Caller ID is set */
    if (j > 0) {
      logger(MSG_INFO, "%s: Call status for %s\n", __func__, phone_number);
      if (memcmp(phone_number, our_phone, 12) == 0) {
        logger(MSG_WARN, "%s: Call status is for us\n", __func__);
        needs_rerouting = 1;
      } else {
        if (get_call_simulation_mode()) {
          logger(MSG_WARN,
                 "%s: Request is for a different number, closing out call\n",
                 __func__);
          close_internal_call(usbfd, pkt->qmi.transaction_id);
        }
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
    ind = NULL;
    break;

  case VO_SVC_GET_ALL_CALL_INFO: /* FIXME: IMPLEMENT THIS */
    logger(MSG_INFO, "%s: VO_SVC_GET_ALL_CALL_INFO: Unimplemented\n", __func__);
    set_log_level(0);
    dump_pkt_raw(bytes, len);
    break;

  case VO_SVC_CALL_STATUS_CHANGE:
    logger(MSG_DEBUG, "%s: State change in call.. on hold?\n", __func__);
    break;

  case VO_SVC_CALL_END_REQ:
    logger(MSG_WARN, "%s: Request to terminate the call\n", __func__);
    if (get_call_simulation_mode()) {
      logger(MSG_DEBUG, "%s: Sending disconnect ACK\n", __func__);
      send_voice_cal_disconnect_ack(usbfd, pkt->qmi.transaction_id);
      usleep(100000);
      logger(MSG_DEBUG, "%s: [DISCONNECTING]\n", __func__);
      close_internal_call(usbfd, pkt->qmi.transaction_id);
      needs_rerouting = 1;
    }
    break;

  default:
    logger(MSG_DEBUG, "%s: Unhandled packet: 0x%.4x\n", __func__, msgid);
    break;
  }

  pkt = NULL;
  return needs_rerouting;
}