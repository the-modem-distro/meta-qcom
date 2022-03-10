#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/audio.h"
#include "../inc/call.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"

struct mixer *mixer;
struct pcm *pcm_tx;
struct pcm *pcm_rx;

/*  Audio runtime state:
 *    current_call_state: IDLE / CIRCUITSWITCH / VOLTE
 *    volte_hd_audio_mode: 8000 / 16000 / 48000
 *    output_device: I2S / USB
 */

struct {
  uint8_t custom_alert_tone;
  uint8_t current_call_state;
  uint8_t volte_hd_audio_mode;
  uint8_t output_device;
  uint8_t is_muted;
  uint8_t is_alerting;
  struct call_data calls[MAX_ACTIVE_CALLS];
  uint8_t is_recording;
} audio_runtime_state;

void set_audio_runtime_default() {
  audio_runtime_state.custom_alert_tone = 0;
  audio_runtime_state.current_call_state = CALL_STATUS_IDLE;
  audio_runtime_state.volte_hd_audio_mode = 0;
  audio_runtime_state.output_device = AUDIO_MODE_I2S;
  audio_runtime_state.is_muted = 0;
  audio_runtime_state.is_alerting = 0;
  audio_runtime_state.is_recording = 0;
}

void set_record(bool en) {
  if (en) {
    audio_runtime_state.is_recording = 1;
  } else {
    audio_runtime_state.is_recording = 0;
  }
}

void configure_custom_alert_tone(bool en) {
  if (en) {
    audio_runtime_state.custom_alert_tone = 1;
  } else {
    audio_runtime_state.custom_alert_tone = 0;
  }
}

int use_external_codec() {
  int fd;
  char buff[32];
  fd = open(EXTERNAL_CODEC_DETECT_PATH, O_RDONLY);
  if (fd < 0) {
    logger(MSG_INFO, "%s: ALC5616 codec not detected \n", __func__);
    return 0;
  }
  close(fd);
  return 1;
}

void set_audio_mute(bool mute) {
  if (audio_runtime_state.current_call_state != CALL_STATUS_IDLE) {
    mixer = mixer_open(SND_CTL);
    if (!mixer) {
      logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno));
      return;
    }

    if (mute) {
      logger(MSG_INFO, "%s: Muting microphone... \n", __func__);

      audio_runtime_state.is_muted = 1;
      switch (audio_runtime_state.output_device) {
      case AUDIO_MODE_I2S: // I2S Audio
        // We close all the mixers
        if (audio_runtime_state.current_call_state == 1) {
          set_mixer_ctl(mixer, RXCTL_VOICE, 0); // Capture
        } else if (audio_runtime_state.current_call_state == 2) {
          set_mixer_ctl(mixer, RXCTL_VOLTE, 0); // Capture
        }
        break;
      case AUDIO_MODE_USB: // USB Audio
        // We close all the mixers
        if (audio_runtime_state.current_call_state == 1) {
          set_mixer_ctl(mixer, AFERX_VOICE, 0); // Capture
        } else if (audio_runtime_state.current_call_state == 2) {
          set_mixer_ctl(mixer, AFERX_VOLTE, 0); // Capture
        }
        break;
      }
    } else {
      logger(MSG_INFO, "%s: Enabling microphone... \n", __func__);

      audio_runtime_state.is_muted = 0;
      switch (audio_runtime_state.output_device) {
      case AUDIO_MODE_I2S: // I2S Audio
        // We close all the mixers
        if (audio_runtime_state.current_call_state == 1) {
          set_mixer_ctl(mixer, RXCTL_VOICE, 1); // Capture
        } else if (audio_runtime_state.current_call_state == 2) {
          set_mixer_ctl(mixer, RXCTL_VOLTE, 1); // Capture
        }
        break;
      case AUDIO_MODE_USB: // USB Audio
        // We close all the mixers
        if (audio_runtime_state.current_call_state == 1) {
          set_mixer_ctl(mixer, AFERX_VOICE, 1); // Capture
        } else if (audio_runtime_state.current_call_state == 2) {
          set_mixer_ctl(mixer, AFERX_VOLTE, 1); // Capture
        }
        break;
      }
    }

    mixer_close(mixer);
  } else {
    audio_runtime_state.is_muted = 0;
    logger(MSG_WARN, "%s: Can't mute audio when there's no call in progress\n",
           __func__);
  }
}

void *play_alerting_tone() {
  char *buffer;
  int size;
  int num_read;
  FILE *file;
  struct pcm *pcm0;
  struct mixer *mymixer;

  /*
   * Ensure we loop the file while alerting
   */
  while (audio_runtime_state.is_alerting) {

    logger(MSG_INFO, "%s: Playing custom alert tone\n", __func__);
    mymixer = mixer_open(SND_CTL);
    if (!mymixer) {
      logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno));
      return NULL;
    }
    set_mixer_ctl(mymixer, MULTIMEDIA_MIXER, 1);
    mixer_close(mymixer);

    pcm0 = pcm_open((PCM_OUT | PCM_MONO), PCM_DEV_HIFI);
    if (pcm0 == NULL) {
      logger(MSG_INFO, "%s: Error opening %s, custom alert tone won't play\n",
             __func__, PCM_DEV_HIFI);
      return NULL;
    }

    pcm0->channels = 1;
    pcm0->flags = PCM_OUT | PCM_MONO;
    pcm0->format = PCM_FORMAT_S16_LE;
    pcm0->rate = 8000;
    pcm0->period_size = 1024;
    pcm0->period_cnt = 1;
    pcm0->buffer_size = 32768;

    file = fopen("/tmp/ring8k.wav", "r");
    if (file == NULL) {
      logger(MSG_INFO, "%s: Falling back to default tone\n", __func__);
      file = fopen("/usr/share/tones/ring8k.wav", "r");
    }

    if (file == NULL) {
      logger(MSG_ERROR, "%s: Unable to open file\n", __func__);
      pcm_close(pcm0);
      return NULL;
    }

    fseek(file, 44, SEEK_SET);

    if (set_params(pcm0, PCM_OUT)) {
      logger(MSG_ERROR, "Error setting TX Params\n");
      pcm_close(pcm0);
      return NULL;
    }

    if (!pcm0) {
      logger(MSG_ERROR, "%s: Unable to open PCM device\n", __func__);
      return NULL;
    }

    size = pcm_frames_to_bytes(pcm0, pcm_get_buffer_size(pcm0));
    buffer = malloc(size * 1024);

    if (!buffer) {
      logger(MSG_ERROR, "Unable to allocate %d bytes\n", size);
      free(buffer);
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
    } while (num_read > 0 && audio_runtime_state.is_alerting);
    fclose(file);
    free(buffer);
    pcm_close(pcm0);

    mymixer = mixer_open(SND_CTL);
    if (!mymixer) {
      logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno));
      return NULL;
    }
    set_mixer_ctl(mymixer, MULTIMEDIA_MIXER, 0);
    mixer_close(mymixer);
  }

  return NULL;
}

void set_output_device(int device) {
  logger(MSG_DEBUG, "%s: Setting audio output to %i \n", __func__, device);
  audio_runtime_state.output_device = device;
}

uint8_t get_output_device() { return audio_runtime_state.output_device; }

void set_auxpcm_sampling_rate(uint8_t mode) {
  int previous_call_state = audio_runtime_state.current_call_state;
  audio_runtime_state.volte_hd_audio_mode = mode;
  if (mode == 1) {
    if (write_to(sysfs_value_pairs[6].path, "16000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting auxpcm_rate to 16k\n", __func__);
    }
    if (use_external_codec() &&
        write_to(sysfs_value_pairs[5].path, "4096000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting clock\n", __func__);
    }
  } else if (mode == 2) {
    if (write_to(sysfs_value_pairs[6].path, "48000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting auxpcm_rate to 48k\n", __func__);
    }
    if (use_external_codec() &&
        write_to(sysfs_value_pairs[5].path, "12288000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting clock\n", __func__);
    }
  } else {
    if (write_to(sysfs_value_pairs[6].path, "8000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting auxpcm_rate to 8k\n", __func__);
    }
    if (use_external_codec() &&
        write_to(sysfs_value_pairs[5].path, "2048000", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting clock\n", __func__);
    }
  }
  // If in call, restart audio
  if (audio_runtime_state.current_call_state != CALL_STATUS_IDLE) {
    stop_audio();
    start_audio(previous_call_state);
  }
}

/* Be careful when logging this as phone numbers will leak if you turn on
   debugging */
void handle_call_pkt(struct call_status_indication *pkt, int sz,
                     uint8_t *phone_number) {
  bool needs_setting_up_paths = false;
  uint8_t direction, state, type, mode, ret, i;
  uint8_t calls_active = 0;
  bool call_in_memory = false;
  // Thread
  pthread_t tone_thread;
  // also need to check pkt 0x24: VOICE_GET_CALL_INFO"

  for (i = 0; i < MAX_ACTIVE_CALLS; i++) {
    if (memcmp(audio_runtime_state.calls[i].phone_number, phone_number,
               MAX_PHONE_NUMBER_LENGTH) == 0) {
      logger(MSG_WARN, "%s: Updating call state for %s\n", __func__,
             phone_number);
      audio_runtime_state.calls[i].direction = pkt->meta.call_direction;
      audio_runtime_state.calls[i].state = pkt->meta.call_state;
      audio_runtime_state.calls[i].call_type = pkt->meta.call_type;
      call_in_memory = true;
    }
    if (audio_runtime_state.calls[i].state != 0x00 && // Not started
        audio_runtime_state.calls[i].state != 0x08 && // Disconnecting
        audio_runtime_state.calls[i].state != 0x09) { // Hanging up
      logger(MSG_INFO, "%s: Call active for %s\n", __func__,
             audio_runtime_state.calls[i].phone_number);
      calls_active++;
    }
  }
  if (!call_in_memory) {
    // find first empty / hung up call and fill it and reset state
    for (i = 0; i < MAX_ACTIVE_CALLS; i++) {
      if (audio_runtime_state.calls[i].state == 0x00 ||
          audio_runtime_state.calls[i].state == 0x09) {
        memset(audio_runtime_state.calls[i].phone_number, 0,
               MAX_PHONE_NUMBER_LENGTH);
        memcpy(audio_runtime_state.calls[i].phone_number, phone_number,
               MAX_PHONE_NUMBER_LENGTH);
        logger(MSG_WARN, "%s: ADDING/REPLACING CALL: %s\n", __func__,
               phone_number);
        audio_runtime_state.calls[i].direction = pkt->meta.call_direction;
        audio_runtime_state.calls[i].state = pkt->meta.call_state;
        audio_runtime_state.calls[i].call_type = pkt->meta.call_type;
        call_in_memory = true;
        break;
      }
    }
  }
  direction = pkt->meta.call_direction;
  state = pkt->meta.call_state;
  type = pkt->meta.call_type;
  logger(MSG_INFO, "%s: TOTAL ACTIVE CALLS: %i\n", __func__, calls_active);
  if (direction == AUDIO_DIRECTION_OUTGOING) {
    logger(MSG_WARN, "%s: Call direction: outgoing \n", __func__);
  } else if (direction == AUDIO_DIRECTION_INCOMING) {
    logger(MSG_WARN, "%s: Call direction: incoming \n", __func__);
  } else {
    logger(MSG_ERROR, "%s: Unknown call direction! \n", __func__);
  }

  switch (type) {
  case CALL_TYPE_NO_NETWORK:
  case CALL_TYPE_UNKNOWN:
  case CALL_TYPE_GSM:
  case CALL_TYPE_UMTS:
  case CALL_TYPE_UNKNOWN_ALT:
    mode = CALL_STATUS_CS;
    logger(MSG_INFO, "%s: Call type: Circuit Switch \n", __func__);
    break;
  case CALL_TYPE_VOLTE:
    mode = CALL_STATUS_VOLTE;
    logger(MSG_INFO, "%s: Call type: VoLTE \n", __func__);
    break;
  default:
    logger(MSG_ERROR, "%s: Unknown call type \n", __func__);
    break;
  }

  switch (state) { // Call status
  case AUDIO_CALL_PREPARING:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: PREPARING, %i \n", __func__, mode);
    start_audio(mode);
    break;
  case AUDIO_CALL_ATTEMPT:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: Attempt call, mode: %i \n", __func__, mode);
    start_audio(mode);
    break;
  case AUDIO_CALL_ORIGINATING:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: ORIGINATING, %i \n", __func__, mode);
    start_audio(mode);
    break;
  case AUDIO_CALL_RINGING:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: RINGING, %i \n", __func__, mode);
    start_audio(mode);
    break;
  case AUDIO_CALL_ESTABLISHED:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: ESTABLISHED, %i \n", __func__, mode);
    start_audio(mode);
    break;
  case AUDIO_CALL_ON_HOLD:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: ON_HOLD, %i \n", __func__, mode);
    break;
  case AUDIO_CALL_WAITING:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: CALL_WAITING, %i \n", __func__, mode);
    break;
  case AUDIO_CALL_ALERTING:
    logger(MSG_INFO, "%s: Call is in alerting state \n", __func__);
    if (!audio_runtime_state.custom_alert_tone) {
      start_audio(mode);
    } else if (audio_runtime_state.custom_alert_tone &&
               !audio_runtime_state.is_alerting) {
      audio_runtime_state.is_alerting = 1;
      stop_audio();
      if ((ret =
               pthread_create(&tone_thread, NULL, &play_alerting_tone, NULL))) {
        logger(MSG_ERROR, "%s: Error creating dialing tone thread\n", __func__);
      }
    } else {
      logger(MSG_INFO,
             "%s: Call is in alerting state but dont know what to do\n",
             __func__);
    }
    break;
  case AUDIO_CALL_DISCONNECTING:
  case AUDIO_CALL_HANGUP:
    if (calls_active == 0) {
      stop_audio();
    }
    audio_runtime_state.is_alerting = 0;
    logger(MSG_INFO, "%s: Disconnecting call (%i) \n", __func__, mode);
    break;

  default:
    audio_runtime_state.is_alerting = 0;
    logger(MSG_ERROR, "%s: Unknown call status %i \n", __func__, state);
    break;
  }
  logger(MSG_INFO,
         "%s:ThisCall: Dir: 0x%.2x Sta: 0x%.2x Typ: 0x%.2x, Mode: 0x%.2x \n",
         __func__, direction, state, type, mode);

  for (i = 0; i < MAX_ACTIVE_CALLS; i++) {
    if (audio_runtime_state.calls[i].state == 0x08 ||
        audio_runtime_state.calls[i].state == 0x09) {
      memset(audio_runtime_state.calls[i].phone_number, 0,
             MAX_PHONE_NUMBER_LENGTH);
      audio_runtime_state.calls[i].state = 0;
      audio_runtime_state.calls[i].call_type = 0;
      audio_runtime_state.calls[i].direction = 0;
    } else if (audio_runtime_state.calls[i].state != 0x00) {
      logger(MSG_INFO, "%s: Number: %s, Dir: 0x%.2x Sta: 0x%.2x Typ: 0x%.2x,\n",
             __func__, audio_runtime_state.calls[i].phone_number,
             audio_runtime_state.calls[i].direction,
             audio_runtime_state.calls[i].state,
             audio_runtime_state.calls[i].call_type);
    }
  }
} // func

int set_mixer_ctl(struct mixer *mixer, char *name, int value) {
  struct mixer_ctl *ctl;
  ctl = get_ctl(mixer, name);
  int r;
  if (!ctl) {
    logger(MSG_ERROR, "%s: Setting %s to value %i failed, cant find control \n",
           __func__, name, value);
    return 0;
  }
  r = mixer_ctl_set_value(ctl, 1, value);
  if (r < 0) {
    logger(MSG_ERROR, "%s: Setting %s to value %i failed \n", __func__, name,
           value);
  }
  return 0;
}

int stop_audio() {
  if (audio_runtime_state.current_call_state == CALL_STATUS_IDLE) {
    logger(MSG_ERROR, "%s: No call in progress \n", __func__);
    return 1;
  }
  if (pcm_tx == NULL || pcm_rx == NULL) {
    logger(MSG_ERROR, "%s: Invalid PCM, did it fail to open?\n", __func__);
    return 1;
  }
  if (audio_runtime_state.is_recording) {
    logger(MSG_ERROR, "%s:Not stopping audio, it was not open\n", __func__);
  }
  if (!audio_runtime_state.is_recording) {

    if (pcm_tx->fd >= 0)
      pcm_close(pcm_tx);
    if (pcm_rx->fd >= 0)
      pcm_close(pcm_rx);
  }
  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno), __LINE__);
    return 0;
  }

  switch (audio_runtime_state.output_device) {
  case AUDIO_MODE_I2S: // I2S Audio
    // We close all the mixers
    if (audio_runtime_state.current_call_state == 1) {
      set_mixer_ctl(mixer, TXCTL_VOICE, 0); // Playback
      set_mixer_ctl(mixer, RXCTL_VOICE, 0); // Capture
    } else if (audio_runtime_state.current_call_state == 2) {
      set_mixer_ctl(mixer, TXCTL_VOLTE, 0); // Playback
      set_mixer_ctl(mixer, RXCTL_VOLTE, 0); // Capture
    }
    break;
  case AUDIO_MODE_USB: // USB Audio
    // We close all the mixers
    if (audio_runtime_state.current_call_state == 1) {
      set_mixer_ctl(mixer, AFETX_VOICE, 0); // Playback
      set_mixer_ctl(mixer, AFERX_VOICE, 0); // Capture
    } else if (audio_runtime_state.current_call_state == 2) {
      set_mixer_ctl(mixer, AFETX_VOLTE, 0); // Playback
      set_mixer_ctl(mixer, AFERX_VOLTE, 0); // Capture
    }
    break;
  }

  mixer_close(mixer);
  audio_runtime_state.current_call_state = CALL_STATUS_IDLE;
  audio_runtime_state.is_muted = 0;

  return 1;
}

/*	Setup mixers and open PCM devs
 *		type: 0: CS Voice Call
 *		      1: VoLTE Call
 * If a call wasn't actually in progress the kernel
 * will complain with ADSP_FAILED / EADSP_BUSY
 */
int start_audio(int type) {
  int i;
  char pcm_device[18];
  if (audio_runtime_state.current_call_state != CALL_STATUS_IDLE &&
      type != audio_runtime_state.current_call_state) {
    logger(MSG_WARN, "%s: Switching audio profiles: 0x%.2x --> 0x%.2x\n",
           __func__, audio_runtime_state.current_call_state, type);
    stop_audio();
  } else if (audio_runtime_state.current_call_state != CALL_STATUS_IDLE &&
             type == audio_runtime_state.current_call_state) {
    logger(MSG_INFO, "%s: Not doing anything, already set.\n", __func__);
    return 0;
  }

  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "%s: Error opening mixer!\n", __func__);
    return 0;
  }
  switch (audio_runtime_state.output_device) {
  case AUDIO_MODE_I2S:
    switch (type) {
    case 1:
      logger(MSG_DEBUG, "Call in progress: Circuit Switch\n");
      set_mixer_ctl(mixer, TXCTL_VOICE, 1); // Playback
      set_mixer_ctl(mixer, RXCTL_VOICE, 1); // Capture
      strncpy(pcm_device, PCM_DEV_VOCS, sizeof(PCM_DEV_VOCS));
      break;
    case 2:
      logger(MSG_DEBUG, "Call in progress: VoLTE\n");
      set_mixer_ctl(mixer, TXCTL_VOLTE, 1); // Playback
      set_mixer_ctl(mixer, RXCTL_VOLTE, 1); // Capture
      strncpy(pcm_device, PCM_DEV_VOLTE, sizeof(PCM_DEV_VOLTE));
      break;
    default:
      logger(MSG_ERROR, "%s: Can't set mixers, unknown call type %i\n",
             __func__, type);
      return -EINVAL;
    }
    break;

  case AUDIO_MODE_USB: // MODE usb
    switch (type) {
    case 1:
      logger(MSG_DEBUG, "Call in progress: Circuit Switch\n");
      set_mixer_ctl(mixer, AFETX_VOICE, 1); // Playback
      set_mixer_ctl(mixer, AFERX_VOICE, 1); // Capture
      strncpy(pcm_device, PCM_DEV_VOCS, sizeof(PCM_DEV_VOCS));
      break;
    case 2:
      logger(MSG_DEBUG, "Call in progress: VoLTE\n");
      set_mixer_ctl(mixer, AFETX_VOLTE, 1); // Playback
      set_mixer_ctl(mixer, AFERX_VOLTE, 1); // Capture
      strncpy(pcm_device, PCM_DEV_VOLTE, sizeof(PCM_DEV_VOLTE));
      break;
    default:
      logger(MSG_ERROR, "%s: Can't set mixers, unknown call type %i\n",
             __func__, type);
      return -EINVAL;
    }
    break;
  }
  mixer_close(mixer);

  pcm_rx = pcm_open((PCM_IN | PCM_MONO | PCM_MMAP), pcm_device);
  pcm_rx->channels = 1;
  pcm_rx->flags = PCM_IN | PCM_MONO;
  pcm_rx->format = PCM_FORMAT_S16_LE;

  pcm_tx = pcm_open((PCM_OUT | PCM_MONO | PCM_MMAP), pcm_device);
  pcm_tx->channels = 1;
  pcm_tx->flags = PCM_OUT | PCM_MONO;
  pcm_tx->format = PCM_FORMAT_S16_LE;

  if (audio_runtime_state.volte_hd_audio_mode == 1) {
    pcm_rx->rate = 16000;
    pcm_tx->rate = 16000;
  } else if (audio_runtime_state.volte_hd_audio_mode == 2) {
    pcm_rx->rate = 48000;
    pcm_tx->rate = 48000;
  } else {
    pcm_rx->rate = 8000;
    pcm_tx->rate = 8000;
  }

  logger(MSG_INFO, "Selected sampling rate: RX: %i, TX: %i\n", pcm_rx->rate,
         pcm_tx->rate);

  if (set_params(pcm_rx, PCM_IN)) {
    logger(MSG_ERROR, "Error setting RX Params\n");
    pcm_close(pcm_rx);
    return -EINVAL;
  }

  if (set_params(pcm_tx, PCM_OUT)) {
    logger(MSG_ERROR, "Error setting TX Params\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
    logger(MSG_ERROR, "Error getting RX PCM ready\n");
    pcm_close(pcm_rx);
    return -EINVAL;
  }

  if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_PREPARE)) {
    logger(MSG_ERROR, "Error getting TX PCM ready\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_tx->fd, SNDRV_PCM_IOCTL_START) < 0) {
    logger(MSG_ERROR, "PCM ioctl start failed for TX\n");
    pcm_close(pcm_tx);
    return -EINVAL;
  }

  if (ioctl(pcm_rx->fd, SNDRV_PCM_IOCTL_START) < 0) {
    logger(MSG_ERROR, "PCM ioctl start failed for RX\n");
    pcm_close(pcm_rx);
  }
  if (audio_runtime_state.is_recording) {
    logger(MSG_WARN, "Closing PCM to be able to record\n");

    pcm_close(pcm_rx);
    pcm_close(pcm_tx);
  }
  if (type == CALL_STATUS_CS || type == CALL_STATUS_VOLTE) {
    audio_runtime_state.current_call_state = type;
  }

  return 0;
}

int dump_audio_mixer() {
  struct mixer *mixer;
  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "%s: Error opening mixer!\n", __func__);
    return -1;
  }
  mixer_dump(mixer);
  mixer_close(mixer);
  return 0;
}

int set_audio_defaults() {
  int i;
  int ret = 0;
  for (i = 0; i < (sizeof(sysfs_value_pairs) / sizeof(sysfs_value_pairs[0]));
       i++) {
    if (write_to(sysfs_value_pairs[i].path, sysfs_value_pairs[i].value,
                 O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             sysfs_value_pairs[i].path);
      ret = -EPERM;
    } else {
      logger(MSG_DEBUG, "%s: Written %s to %s \n", __func__,
             sysfs_value_pairs[i].value, sysfs_value_pairs[i].path);
    }
  }
  return ret;
}

int set_external_codec_defaults() {
  int i;
  int ret = 0;
  for (i = 0; i < (sizeof(alc5616_default_settings) /
                   sizeof(alc5616_default_settings[0]));
       i++) {
    if (write_to(alc5616_default_settings[i].path,
                 alc5616_default_settings[i].value, O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error writing to %s\n", __func__,
             alc5616_default_settings[i].path);
      ret = -EPERM;
    } else {
      logger(MSG_DEBUG, "%s: Written %s to %s \n", __func__,
             alc5616_default_settings[i].value,
             alc5616_default_settings[i].path);
    }
  }
  set_auxpcm_sampling_rate(1); // Set audio mode to 48KPCM
  return ret;
}
