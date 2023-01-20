// SPDX-License-Identifier: MIT

#include "../inc/call.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/command.h"
#include "../inc/config.h"
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
  uint8_t do_not_disturb;
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

void set_do_not_disturb(bool en) {
  if (en) {
    call_rt.do_not_disturb = 1;
  } else {
    call_rt.do_not_disturb = 0;
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
  call_rt.do_not_disturb = 0;
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
  logger(MSG_DEBUG, "%s: Sending Call Request Response\n", __func__);
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
  logger(MSG_DEBUG,
         "%s: Send QMI call status message: Direction %i, state %i\n", __func__,
         call_direction, call_state);
  uint8_t phone_proto[] = {0x2b, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34,
                           0x35, 0x35, 0x36, 0x36, 0x37, 0x37};
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
  pkt->meta.num_instances = 0x01;
  pkt->meta.call_id = 0x01;
  pkt->meta.call_state = call_state;
  pkt->meta.call_direction = call_direction;
  pkt->meta.call_mode = CALL_MODE_VOLTE;
  pkt->meta.is_multiparty = 0x00;
  pkt->meta.als = 0x00;

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
  logger(MSG_DEBUG, "%s: Send QMI call ACCEPT message\n", __func__);
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
                               CALL_STATE_DISCONNECTING);
  usleep(100000);
  send_voice_call_status_event(usbfd, transaction_id + 1,
                               call_rt.sim_call_direction, CALL_STATE_HANGUP);

  set_call_simulation_mode(false);
  call_rt.sim_call_direction = 0;
}

void start_simulated_call(int usbfd) {
  call_rt.transaction_id = 0x01;
  call_rt.simulated_call_state = CALL_STATE_RINGING;
  call_rt.sim_call_direction = CALL_DIRECTION_INCOMING;
  set_call_simulation_mode(true);
  pulse_ring_in();
  usleep(300000); // try with 300ms
  logger(MSG_WARN, " Call update notification: RINGING to %x\n", usbfd);
  send_voice_call_status_event(usbfd, call_rt.transaction_id,
                               CALL_DIRECTION_INCOMING, CALL_STATE_RINGING);
}

uint8_t send_voice_cal_disconnect_ack(int usbfd, uint16_t transaction_id) {
  struct end_call_response *pkt;
  logger(MSG_DEBUG, "%s: Send QMI call END message\n", __func__);
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

  if (call_rt.simulated_call_state != CALL_STATE_ESTABLISHED) {
    if (call_rt.timeout_counter > 60) {
      logger(MSG_INFO, "%s: Killing internal call\n", __func__);
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
  struct pcm *pcm0 = NULL;
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
    free(buffer);
  }

  logger(MSG_INFO, "%s: Cleaning up\n", __func__);
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
  send_voice_call_status_event(usbfd, transaction_id + 1,
                               CALL_DIRECTION_INCOMING, CALL_STATE_ESTABLISHED);
  usleep(100000);
  if ((ret = pthread_create(&dummy_echo_thread, NULL,
                            &simulated_call_tts_handler, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }
}

void send_dummy_call_established(int usbfd, uint16_t transaction_id) {
  pthread_t dummy_echo_thread;
  int ret;
  logger(MSG_DEBUG, "%s: Sending response: ATTEMPT\n", __func__);
  send_voice_call_status_event(usbfd, transaction_id, CALL_DIRECTION_OUTGOING,
                               CALL_STATE_ATTEMPT);
  usleep(100000);
  logger(MSG_DEBUG, "%s: Sending response: Call request ACK \n", __func__);
  send_call_request_response(usbfd, transaction_id);
  usleep(100000);
  logger(MSG_DEBUG, "%s: Sending response: ALERTING\n", __func__);
  transaction_id++;
  send_voice_call_status_event(usbfd, transaction_id, CALL_DIRECTION_OUTGOING,
                               CALL_STATE_ALERTING);
  usleep(100000);
  logger(MSG_DEBUG, "%s: Sending response: ESTABLISHED\n", __func__);
  transaction_id++;
  send_voice_call_status_event(usbfd, transaction_id, CALL_DIRECTION_OUTGOING,
                               CALL_STATE_ESTABLISHED);
  usleep(100000);
  if ((ret = pthread_create(&dummy_echo_thread, NULL,
                            &simulated_call_tts_handler, NULL))) {
    logger(MSG_ERROR, "%s: Error creating echo thread\n", __func__);
  }

  call_rt.transaction_id = transaction_id; // Save the current transaction ID
}

uint8_t get_num_instances(void *bytes, size_t len) {
  uint8_t instances = 0;
  struct call_status_meta *meta;
  int offset = get_tlv_offset_by_id((uint8_t *)bytes, len, TLV_CALL_INFO);
  if (offset <= 0) {
    logger(MSG_ERROR, "%s:Couldn't retrieve call metadata \n", __func__);
  } else if (offset > 0) {
    meta = (struct call_status_meta *)(bytes + offset);
    instances = meta->num_instances;
    meta = NULL;
  }
  return instances;
}

/*
  Returns the current call state from a
  VO_SVC_CALL_STATUS QMI Indication message (preparing/alerting/ringing...)
*/
uint8_t get_call_state(void *bytes, size_t len, uint8_t tlv) {
  uint8_t state = 0;
  struct call_status_meta *meta;
  int offset = get_tlv_offset_by_id((uint8_t *)bytes, len, tlv);
  if (offset <= 0) {
    logger(MSG_ERROR, "%s:Couldn't retrieve call metadata \n", __func__);
  } else if (offset > 0) {
    meta = (struct call_status_meta *)(bytes + offset);
    state = meta->call_state;
    meta = NULL;
  }
  return state;
}

// For user automatic call termination on call waiting
void autokill_call(void *bytes, size_t len, uint8_t call_id, int adspfd) {
  logger(MSG_WARN, "%s: Automaticall killing Waiting call with ID %i\n",
         __func__, call_id);
  struct encapsulated_qmi_packet *pkt = (struct encapsulated_qmi_packet *)bytes;
  struct call_hangup_request *request;
  request = calloc(1, sizeof(struct call_hangup_request));
  request->qmuxpkt.version = 0x01;
  request->qmuxpkt.packet_length = 0x10;
  request->qmuxpkt.control = 0x00;
  request->qmuxpkt.service = 0x09;
  request->qmuxpkt.instance_id = 0x02;
  request->qmipkt.ctlid = 0x00;
  request->qmipkt.transaction_id = pkt->qmi.transaction_id + 1;
  request->qmipkt.msgid = VO_SVC_CALL_END_REQ;
  request->qmipkt.length = 0x04;
  request->call_id.id = 0x01;
  request->call_id.len = 0x01;
  request->call_id.data = call_id;

  logger(MSG_INFO, "%s: Sending the payload to the baseband\n", __func__);
  if (write(adspfd, request, sizeof(struct call_hangup_request)) < 0) {
    logger(MSG_INFO, "%s: Error sending payload\n", __func__);
  } else {
    logger(MSG_INFO, "%s: Payload *SENT*\n", __func__);
  }

  free(request);
  request = NULL;
  pkt = NULL;
}

/*
  Returns CALL_DIRECTION_OUTGOING or CALL_DIRECTION_INCOMING
  from a given VO_SVC_CALL_STATUS QMI Indication message
*/
uint8_t get_call_direction(uint8_t *pkt, int sz, uint8_t tlv) {
  uint8_t call_direction = 0;
  int offset = get_tlv_offset_by_id((uint8_t *)pkt, sz, tlv);
  if (offset <= 0) {
    logger(MSG_ERROR, "%s:Couldn't retrieve call metadata \n", __func__);
  } else if (offset > 0) {
    struct call_status_meta *meta;
    meta = (struct call_status_meta *)(pkt + offset);
    call_direction = meta->call_direction;
    meta = NULL;
  }
  return call_direction;
}

uint8_t handle_voice_service_call_request(uint8_t source, void *bytes,
                                          size_t len, int adspfd, int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;
  uint8_t phone_num_size = 0;
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH] = {0};
  char log_phone_number[MAX_PHONE_NUMBER_LENGTH] = {0};
  char our_phone[] = "223344556677";
  struct call_request_indication *req = (struct call_request_indication *)bytes;

  for (int i = 0; i < req->number.phone_num_length; i++) {
    if (req->number.phone_number[i] >= 0x30 &&
        req->number.phone_number[i] <= 0x39) {
      phone_number[phone_num_size] = req->number.phone_number[i];
      phone_num_size++;
    }
  }
  mask_phone_number(phone_number, log_phone_number, phone_num_size);

  if (phone_num_size > 0 && memcmp(phone_number, our_phone, 12) == 0) {
    logger(MSG_INFO, "%s: This call is for me: %s\n", __func__,
           log_phone_number);
    if (!get_call_simulation_mode()) {
      set_call_simulation_mode(true);
      send_dummy_call_established(usbfd, req->qmipkt.transaction_id);
      call_rt.sim_call_direction = CALL_DIRECTION_OUTGOING;
    } else {
      logger(MSG_ERROR, "%s: We're already talking!\n", __func__);
    }
    proxy_action = PACKET_BYPASS;
  } else {
    logger(MSG_INFO, "%s: Outgoing call to %s\n", __func__, log_phone_number);
    if (get_call_simulation_mode()) {
      logger(MSG_WARN, "%s: Ending internal call first\n", __func__);
      close_internal_call(usbfd, get_qmi_transaction_id(bytes, len));
    }
  }
  req = NULL;
  return proxy_action;
}

uint8_t handle_voice_service_answer_request(uint8_t source, void *bytes,
                                            size_t len, int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;
  if (get_call_simulation_mode()) {
    logger(MSG_INFO, "%s: Admin wants to talk!\n", __func__);
    process_incoming_call_accept(usbfd, get_qmi_transaction_id(bytes, len));
    proxy_action = PACKET_BYPASS;
  }

  return proxy_action;
}

uint8_t handle_voice_service_disconnect_request(uint8_t source, void *bytes,
                                                size_t len, int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;
  logger(MSG_INFO, "%s: Request to terminate the call\n", __func__);
  if (get_call_simulation_mode()) {
    logger(MSG_DEBUG, "%s: Sending disconnect ACK\n", __func__);
    send_voice_cal_disconnect_ack(usbfd, get_qmi_transaction_id(bytes, len));
    usleep(100000);
    logger(MSG_DEBUG, "%s: [DISCONNECTING]\n", __func__);
    close_internal_call(usbfd, get_qmi_transaction_id(bytes, len));
    proxy_action = PACKET_BYPASS;
  }

  return proxy_action;
}

uint8_t handle_voice_service_call_info(uint8_t source, void *bytes, size_t len,
                                       int adspfd, int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;

  switch (source) {
  case FROM_DSP:
    logger(MSG_INFO, "%s DSP is sending a call info response\n", __func__);
    break;
  case FROM_HOST:
    logger(MSG_INFO, "%s HOST requests call information\n", __func__);
    break;
  }
  /* NOT IMPLEMENTED */
  logger(MSG_INFO, "%s: VO_SVC_CALL_INFO: Unimplemented!\n", __func__);
  set_log_level(MSG_DEBUG);
  dump_pkt_raw(bytes, len);
  set_log_level(MSG_INFO);
  if (source == FROM_DSP && call_rt.do_not_disturb &&
      get_call_direction(bytes, len, TLV_REMOTE_NUMBER) ==
          CALL_DIRECTION_INCOMING) {
    proxy_action = PACKET_BYPASS;
    logger(MSG_INFO, "CALL RING BYPASS FROM ALL_CALL_INFO!\n");
  }
  return proxy_action;
}

uint8_t handle_voice_service_call_status(uint8_t source, void *bytes,
                                         size_t len, int adspfd, int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;
  uint16_t offset;
  uint8_t phone_number[MAX_PHONE_NUMBER_LENGTH] = {0};
  char log_phone_number[MAX_PHONE_NUMBER_LENGTH] = {0};
  char our_phone[] = "223344556677";
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  int j = 0;
  switch (source) {
  case FROM_DSP:
    logger(MSG_INFO, "%s DSP is sending a call status response\n", __func__);
    break;
  case FROM_HOST:
    logger(MSG_INFO, "%s HOST requests call status\n", __func__);
    break;
  }

  offset = get_tlv_offset_by_id((uint8_t *)bytes, len, TLV_REMOTE_NUMBER);
  if (offset == 0) {
    logger(MSG_ERROR,
           "%s:CRITICAL: Couldn't find the remote party data in the QMI "
           "message \n",
           __func__);
    return proxy_action;
  }

  struct remote_party *rmtparty = (struct remote_party *)(bytes + offset);
  logger(MSG_DEBUG, "Size %i , Call instances: %i\n", rmtparty->length,
         get_num_instances(bytes, len));
  struct remote_party_data *thisnum;
  int prev_num_size = 0;
  for (int i = 0; i < get_num_instances(bytes, len); i++) {
    if (i == 0) {
      /* We start at remote_party + id + len + first_entry */
      thisnum = (struct remote_party_data *)(bytes + offset + 3);
      memcpy(phone_number, thisnum->phone, thisnum->len);
      mask_phone_number(phone_number, log_phone_number, thisnum->len);
      logger(MSG_INFO, "%s: Call #%i: %s\n", __func__, i, log_phone_number);
      prev_num_size += thisnum->len + 3;

      if (get_num_instances(bytes, len) > 1 &&
          get_call_state(bytes, len, TLV_CALL_INFO) == CALL_STATE_WAITING) {
        if (callwait_auto_hangup_operation_mode() == 2) {
          int strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                               "Automatically rejecting the call from %s "
                               "while you're talking\n",
                               phone_number);
          add_sms_to_queue(reply, strsz);
          autokill_call(bytes, len, thisnum->call_id, adspfd);
          proxy_action = PACKET_BYPASS;
        } else if (callwait_auto_hangup_operation_mode() == 1) {
          int strsz =
              snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "Call from %s while you're talking (ignoring it) \n",
                       phone_number);
          add_sms_to_queue(reply, strsz);
          proxy_action = PACKET_BYPASS;
        }
      } else if (call_rt.do_not_disturb &&
                 get_call_direction(bytes, len, TLV_CALL_INFO) ==
                     CALL_DIRECTION_INCOMING &&
                 get_call_state(bytes, len, TLV_CALL_INFO) ==
                     CALL_STATE_RINGING) {
        int strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                             "Call from %s while in DND mode \n", phone_number);
        add_sms_to_queue(reply, strsz);
        proxy_action = PACKET_BYPASS;
        logger(MSG_INFO, "CALL RING BYPASS!\n");
      }
    } else {
      thisnum =
          (struct remote_party_data *)(bytes + offset + 3 + prev_num_size);
      memcpy(phone_number, thisnum->phone, thisnum->len);
      mask_phone_number(phone_number, log_phone_number, thisnum->len);
      logger(MSG_INFO, "%s: Call #%i: %s\n", __func__, i, log_phone_number);
      if (callwait_auto_hangup_operation_mode() > 0) {
        proxy_action = PACKET_BYPASS;
      }
    }
    j = thisnum->len;
    thisnum = NULL;
  }
  rmtparty = NULL;

  if (call_rt.do_not_disturb && get_call_direction(bytes, len, TLV_CALL_INFO) ==
                                    CALL_DIRECTION_INCOMING) {
    proxy_action = PACKET_BYPASS;
    logger(MSG_INFO, "We're in DND mode. Skip notifying the host\n");
    return proxy_action;
  }

  /* Caller ID is set */
  if (j > 0) {
    if (memcmp(phone_number, our_phone, 12) == 0) {
      logger(MSG_DEBUG, "%s: Internal call\n", __func__);
      proxy_action = PACKET_BYPASS;
    } else {
      if (get_call_simulation_mode()) {
        logger(MSG_WARN,
               "%s: New call while internally talking, ending simulated call\n",
               __func__);
        close_internal_call(usbfd, get_qmi_transaction_id(bytes, len));
      }
      handle_call_pkt(bytes, len, phone_number, strlen((char *)phone_number));
    }
  } else {
    /* Caller ID is *NOT* set */
    if (get_call_simulation_mode()) {
      proxy_action = PACKET_BYPASS;
    } else {
      logger(MSG_WARN, "%s: Unknown number %s\n", __func__, log_phone_number);
      memcpy((uint8_t *)phone_number, (uint8_t *)"Unknown", strlen("Unknown"));
      handle_call_pkt(bytes, len, phone_number, strlen((char *)phone_number));
    }
  }
  return proxy_action;
}

uint8_t handle_voice_service_all_call_status_info(uint8_t source, void *bytes,
                                                  size_t len, int adspfd,
                                                  int usbfd) {
  uint8_t proxy_action = PACKET_FORCED_PT;
  switch (source) {
  case FROM_DSP:
    logger(MSG_INFO, "%s DSP is sending a all call info response\n", __func__);
    if (call_rt.do_not_disturb) {
      size_t response_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) + sizeof (struct qmi_generic_result_ind);
      uint8_t *fake_response = malloc(response_len);
      logger(MSG_INFO, "Killing this packet\n");
      if (build_qmux_header(fake_response, response_len, 0x80, get_qmux_service_id(bytes, len), get_qmux_instance_id(bytes, len)) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
      }
      if (build_qmi_header(fake_response, response_len, 0x02, get_qmi_transaction_id(bytes, len),
                            VO_SVC_GET_ALL_CALL_INFO) < 0) {
        logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
      }
      struct qmi_generic_result_ind* indication = (struct qmi_generic_result_ind*) (fake_response + response_len - sizeof(struct qmi_generic_result_ind));
      indication->result_code_type = TLV_QMI_RESULT;
      indication->generic_result_size = 0x04;
      indication->result = 0x00;
      indication->response = 0x00;
      if (write(usbfd, fake_response, response_len) != response_len) {
        logger(MSG_ERROR, "%s: Error writing simulated response\n", __func__);
      }
      free(fake_response);
      proxy_action = PACKET_BYPASS;
    }
    break;
  case FROM_HOST:
    logger(MSG_INFO,
           "%s HOST requests call information for *all* active calls\n",
           __func__);
    break;
  }
  return proxy_action;
}

uint8_t call_service_handler(uint8_t source, void *bytes, size_t len,
                             int adspfd, int usbfd) {
  int proxy_action = PACKET_FORCED_PT;

  uint16_t qmi_message_id = get_qmi_message_id(bytes, len);

  switch (qmi_message_id) {
  case VO_SVC_CALL_REQUEST:
    proxy_action =
        handle_voice_service_call_request(source, bytes, len, adspfd, usbfd);
    break;

  case VO_SVC_CALL_ANSWER_REQ:
    proxy_action =
        handle_voice_service_answer_request(source, bytes, len, usbfd);
    break;

  case VO_SVC_CALL_INFO: /* FIXME: IMPLEMENT THIS */
    proxy_action =
        handle_voice_service_call_info(source, bytes, len, adspfd, usbfd);
    break;

  case VO_SVC_CALL_STATUS:
    proxy_action =
        handle_voice_service_call_status(source, bytes, len, adspfd, usbfd);
    break;

  case VO_SVC_GET_ALL_CALL_INFO: /* FIXME: IMPLEMENT THIS */
    proxy_action = handle_voice_service_all_call_status_info(source, bytes, len,
                                                             adspfd, usbfd);
    break;

  case VO_SVC_CALL_STATUS_CHANGE:
    logger(MSG_INFO, "%s: VO_SVC_CALL_STATUS_CHANGE: Unimplemented\n",
           __func__);
    set_log_level(MSG_DEBUG);
    dump_pkt_raw(bytes, len);
    set_log_level(MSG_INFO);
    break;

  case VO_SVC_CALL_END_REQ:
    proxy_action =
        handle_voice_service_disconnect_request(source, bytes, len, usbfd);
    break;

  default:
    logger(MSG_DEBUG, "%s: Unhandled packet: 0x%.4x (let it pass...)\n", __func__,
           qmi_message_id);
    break;
  }
  return proxy_action;
}