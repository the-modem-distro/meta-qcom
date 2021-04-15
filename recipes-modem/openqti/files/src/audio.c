#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"

struct mixer *mixer;
struct pcm *pcm_tx;
struct pcm *pcm_rx;
uint8_t current_call_state;

/* Be careful when logging this as phone numbers will leak if you turn on
   debugging */
void handle_call_pkt(uint8_t *pkt, int from, int sz) {
  if (sz > 16 && from == FROM_DSP) {
    if (pkt[0] == 0x1 && pkt[2] == 0x00 && pkt[3] == 0x80 && pkt[4] == 0x09) {
      switch (pkt[1]) {
      case 0x38:
        logger(MSG_WARN, "%s: Outgoing Call start: CS Voice\n", __func__);
        start_audio(CALL_MODE_CS);
        break;
      case 0x3c: // or 0x42
        logger(MSG_WARN, "%s: Incoming Call start : VoLTE\n", __func__);
        start_audio(CALL_MODE_VOLTE);
        break;
      case 0x40:
        logger(MSG_WARN, "%s: Outgoing Call start: VoLTE\n", __func__);
        start_audio(CALL_MODE_VOLTE);
        break;
      case 0x46:
        logger(MSG_WARN, "%s: Incoming Call start : CS Voice\n", __func__);
        start_audio(CALL_MODE_CS); // why did I set this to VoLTE previously?
        break;
      case 0x41:
      case 0x43: // VoLTE Hangup in HD Voice
      case 0x5f: // Hangup from LTE?
      case 0x63: // Hangup unanswered (NO CARRIER)
      case 0x69: // Hangup from CSVoice
        logger(MSG_WARN, "%s: Call finished \n", __func__);
        stop_audio();
        break;
      }
    }
  }
}

int set_mixer_ctl(struct mixer *mixer, char *name, int value) {
  struct mixer_ctl *ctl;
  ctl = get_ctl(mixer, name);
  int r;

  r = mixer_ctl_set_value(ctl, 1, value);
  if (r < 0) {
    logger(MSG_ERROR, "%s: Setting %s to value %i failed \n", __func__, name,
           value);
  }
  return 0;
}

int stop_audio() {
  if (current_call_state == 0) {
    logger(MSG_ERROR, "%s: No call in progress \n", __func__);
    return 1;
  }
  if (pcm_tx == NULL || pcm_rx == NULL) {
    logger(MSG_ERROR, "%s: Invalid PCM, did it fail to open?\n", __func__);
  }
  if (pcm_tx->fd >= 0)
    pcm_close(pcm_tx);
  if (pcm_rx->fd >= 0)
    pcm_close(pcm_rx);

  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno), __LINE__);
    return 0;
  }

  // We close all the mixers
  if (current_call_state == 1) {
    set_mixer_ctl(mixer, TXCTL_VOICE, 0); // Playback
    set_mixer_ctl(mixer, RXCTL_VOICE, 0); // Capture
  } else if (current_call_state == 2) {
    set_mixer_ctl(mixer, TXCTL_VOLTE, 0); // Playback
    set_mixer_ctl(mixer, RXCTL_VOLTE, 0); // Capture
  }

  mixer_close(mixer);
  current_call_state = 0;
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
  if (current_call_state > 0 && type != current_call_state) {
    logger(MSG_ERROR,
           "%s: Audio already active for other profile, restarting... \n",
           __func__);
    stop_audio();
  } else if (current_call_state > 0 && type == current_call_state) {
    logger(MSG_WARN, "%s: Call already setup and enabled, nothing to do\n",
           __func__);
    return 0;
  }

  mixer = mixer_open(SND_CTL);
  if (!mixer) {
    logger(MSG_ERROR, "%s: Error opening mixer!\n", __func__);
    return 0;
  }

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
    logger(MSG_ERROR, "%s: Can't set mixers, unknown call type %i\n", __func__,
           type);
    return -EINVAL;
  }
  mixer_close(mixer);

  pcm_rx = pcm_open((PCM_IN | PCM_MONO), pcm_device);
  pcm_rx->channels = 1;
  pcm_rx->rate = 8000;
  pcm_rx->flags = PCM_IN | PCM_MONO;

  pcm_tx = pcm_open((PCM_OUT | PCM_MONO), pcm_device);
  pcm_tx->channels = 1;
  pcm_tx->rate = 8000;
  pcm_tx->flags = PCM_OUT | PCM_MONO;

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

  if (type == 1) {
    current_call_state = 1;
  } else if (type == 2) {
    current_call_state = 2;
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
