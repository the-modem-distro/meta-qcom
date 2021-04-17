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
  bool needs_setting_up_paths = false;

  if (sz > 25 && from == FROM_DSP) {
    /* What are we looking for? A voice service QMI packet with the following params:
    - frame 0x01
    - with flag 0x80 
    - of service 0x09 (voice svc)
    - and pkt type 0x04
    - and msg ID 0x2e (call indication)
    */
    if (pkt[0] == 0x1 && pkt[3] == 0x80 && 
        pkt[4] == 0x09 && pkt[6] == 0x04 &&
        pkt[9] == 0x2e) {

      if (pkt[20] == 0x01) {
        logger(MSG_WARN, "%s: Call direction: outgoing \n", __func__);
      } else if (pkt[20] == 0x02) {
        logger(MSG_WARN, "%s: Call direction: incoming \n", __func__);
      } else {
       logger(MSG_ERROR, "%s: Unknown call direction! \n", __func__);
      }

      switch (pkt[18]) { // Call state
      case 0x01:
        logger(MSG_WARN, "%s: State: originating\n", __func__);
        needs_setting_up_paths = true;
        break;
      case 0x02:
        logger(MSG_WARN, "%s: State: Ringing\n", __func__);
        needs_setting_up_paths = true;
        break;
      case 0x03:
        logger(MSG_WARN, "%s: State: Call in progress \n", __func__);
        needs_setting_up_paths = true;

        break;
      case 0x04:
        logger(MSG_WARN, "%s: State: Trying to call...\n", __func__);
        needs_setting_up_paths = true;
        break;
      case 0x05:
        logger(MSG_WARN, "%s: State: Ringing?\n", __func__);
        break;
      case 0x06:
        logger(MSG_WARN, "%s: State: Hold\n", __func__);
        break;
      case 0x07:
        logger(MSG_WARN, "%s: State: Waiting?\n", __func__);
        break;
      case 0x08:
        logger(MSG_WARN, "%s: State: Disconnecting call\n", __func__);
        stop_audio();
        break;
      case 0x09:
        logger(MSG_WARN, "%s: State: Hang up! \n", __func__);
        break;
      case 0x0a:
        logger(MSG_WARN, "%s: State: Preparing for outgoing... \n", __func__);
        break;
      default:
        logger(MSG_WARN, "%s: State: Unknown pkt 0x%.2x", __func__, pkt[18]);
        break;
      } // end of call state

      if (needs_setting_up_paths) {
        switch (pkt[21]) {
        case 0x00:
          logger(MSG_WARN, "%s: Set audio paths: No network? Try with CS... \n", __func__);
          start_audio(CALL_MODE_CS);

          break;
        case 0x01:
          logger(MSG_WARN, "%s:  Set audio paths: Unknown mode, Try with CS... \n", __func__);
          start_audio(CALL_MODE_CS);

          break;
        case 0x02:
          logger(MSG_WARN, "%s:  Set audio paths: GSM Call mode \n", __func__);
          start_audio(CALL_MODE_CS);

          break;
        case 0x03:
          logger(MSG_WARN, "%s:  Set audio paths:  UMTS Call mode \n", __func__);
          start_audio(CALL_MODE_CS);

          break;
        case 0x04:
          logger(MSG_WARN, "%s:  Set audio paths: VoLTE Call mode \n", __func__);
          start_audio(CALL_MODE_VOLTE);
          break;
        case 0x05:
          logger(MSG_WARN, "%s: Set audio paths: Can't call with no network!\n", __func__);
          break;
        default:
          logger(MSG_WARN, "%s: Set audio paths: Can't guess call type! \n", __func__);
          break;
        } // end of call method
      } // if needs setting up paths
    } // if packet is call indication, and comes from dsp
  } // length
} //func

  int set_mixer_ctl(struct mixer * mixer, char *name, int value) {
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
      logger(MSG_ERROR, "error opening mixer! %s:\n", strerror(errno),
             __LINE__);
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
      logger(MSG_ERROR, "%s: Can't set mixers, unknown call type %i\n",
             __func__, type);
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
