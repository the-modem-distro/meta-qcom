/* SPDX-License-Identifier: MIT */

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sound/asound.h>
#include <stdbool.h>
#include <stdint.h>

/* Mixers used in VoLTE / VoLTE HD */
#define RXCTL_VOLTE "SEC_AUX_PCM_RX_Voice Mixer VoLTE"
#define TXCTL_VOLTE "VoLTE_Tx Mixer SEC_AUX_PCM_TX_VoLTE"

/* Mixers used in Circuit Switch type voice calls (2G/3G) */
#define RXCTL_VOICE "SEC_AUX_PCM_RX_Voice Mixer CSVoice"
#define TXCTL_VOICE "Voice_Tx Mixer SEC_AUX_PCM_TX_Voice"

/* Mixers used in VoLTE / VoLTE HD calls with USB Audio */
#define AFECTL_VOLTE "SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX"
#define AFERX_VOLTE "AFE_PCM_RX_Voice Mixer VoLTE"
#define AFETX_VOLTE "VoLTE_Tx Mixer AFE_PCM_TX_VoLTE"

/* Mixers used in Circuit Switch type voice calls (2G/3G) with USB Audio */
#define AFECTL_VOICE "SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX"
#define AFERX_VOICE "AFE_PCM_RX_Voice Mixer CSVoice"
#define AFETX_VOICE "Voice_Tx Mixer AFE_PCM_TX_Voice"

#define MULTIMEDIA_MIXER "SEC_AUX_PCM_RX Audio Mixer MultiMedia1"
#define AUDIO_MODE_I2S 0//SEC_AUX_PCM_RX Audio Mixer
#define AUDIO_MODE_USB 1

enum call_direction {
  AUDIO_DIRECTION_OUTGOING = 0x01,
  AUDIO_DIRECTION_INCOMING = 0x02,
};

enum call_status {
  AUDIO_CALL_ORIGINATING = 0x01,
  AUDIO_CALL_RINGING = 0x02,
  AUDIO_CALL_ESTABLISHED = 0x03,
  AUDIO_CALL_ATTEMPT = 0x04,
  AUDIO_CALL_ALERTING = 0x05,
  AUDIO_CALL_ON_HOLD = 0x06,
  AUDIO_CALL_WAITING = 0x07,
  AUTIO_CALL_DISCONNECTING = 0x08,
  AUDIO_CALL_HANGUP = 0x09,
  AUDIO_CALL_PREPARING = 0x0a
};

enum call_type {
  CALL_TYPE_NO_NETWORK = 0x00,
  CALL_TYPE_UNKNOWN = 0x01,
  CALL_TYPE_GSM = 0x02,
  CALL_TYPE_UMTS = 0x03,
  CALL_TYPE_VOLTE = 0x04,
  CALL_TYPE_UNKNOWN_ALT = 0x05 // this I don't know what it is
};

/* AUDIO */

enum ctl_type {
  CTL_GLOBAL_VOLUME,
  CTL_PLAYBACK_VOLUME,
  CTL_CAPTURE_VOLUME,
};

struct mixer_ctl {
  struct mixer *mixer;
  struct snd_ctl_elem_info *info;
  char **ename;
};

enum snd_pcm_stream_t {
  /** Playback stream */
  SND_PCM_STREAM_PLAYBACK = 0,
  /** Capture stream */
  SND_PCM_STREAM_CAPTURE,
  SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE
};

enum _snd_ctl_elem_iface {
  /** Card level */
  SND_CTL_ELEM_IFACE_CARD = 0,
  /** Hardware dependent device */
  SND_CTL_ELEM_IFACE_HWDEP,
  /** Mixer */
  SND_CTL_ELEM_IFACE_MIXER,
  /** PCM */
  SND_CTL_ELEM_IFACE_PCM,
  /** RawMidi */
  SND_CTL_ELEM_IFACE_RAWMIDI,
  /** Timer */
  SND_CTL_ELEM_IFACE_TIMER,
  /** Sequencer */
  SND_CTL_ELEM_IFACE_SEQUENCER,
  SND_CTL_ELEM_IFACE_LAST = SND_CTL_ELEM_IFACE_SEQUENCER
};

struct mixer {
  int fd;
  struct snd_ctl_elem_info *info;
  struct mixer_ctl *ctl;
  unsigned count;
};

static const struct suf {
  const char *suffix;
  snd_ctl_elem_iface_t type;
} suffixes[] = {{" Playback Volume", CTL_PLAYBACK_VOLUME},
                {" Capture Volume", CTL_CAPTURE_VOLUME},
                {" Volume", CTL_GLOBAL_VOLUME},
                {NULL, 0}};

/* PCM */
#define PCM_ERROR_MAX 128
#define PARAM_MAX SNDRV_PCM_HW_PARAM_LAST_INTERVAL
#define PATH_RX 1
#define PATH_TX 2
struct pcm {
  int fd;
  int timer_fd;
  unsigned rate;
  unsigned channels;
  unsigned flags;
  unsigned format;
  unsigned running:1;
  int underruns;
  unsigned buffer_size;
  unsigned period_size;
  unsigned period_cnt;
  char error[PCM_ERROR_MAX];
  struct snd_pcm_hw_params *hw_p;
  struct snd_pcm_sw_params *sw_p;
  struct snd_pcm_sync_ptr *sync_ptr;
  struct snd_pcm_channel_info ch[2];
  void *addr;
  int card_no;
  int device_no;
  int start;
};
#define FORMAT(v) SNDRV_PCM_FORMAT_##v

#define PCM_OUT 0x00000000
#define PCM_IN 0x10000000

#define PCM_STEREO 0x00000000
#define PCM_MONO 0x01000000
#define PCM_QUAD 0x02000000
#define PCM_5POINT1 0x04000000
#define PCM_7POINT1 0x08000000

#define PCM_44100HZ 0x00000000
#define PCM_48000HZ 0x00100000
#define PCM_8000HZ 0x00200000
#define PCM_RATE_MASK 0x00F00000

#define PCM_MMAP 0x00010000
#define PCM_NMMAP 0x00000000

#define DEBUG_ON 0x00000001
#define DEBUG_OFF 0x00000000

#define PCM_PERIOD_CNT_MIN 2
#define PCM_PERIOD_CNT_SHIFT 16
#define PCM_PERIOD_CNT_MASK (0xF << PCM_PERIOD_CNT_SHIFT)
#define PCM_PERIOD_SZ_MIN 128
#define PCM_PERIOD_SZ_SHIFT 12
#define PCM_PERIOD_SZ_MASK (0xF << PCM_PERIOD_SZ_SHIFT)

/* Bit formats */
enum pcm_format {
    PCM_FORMAT_S16_LE = 0,
    PCM_FORMAT_S32_LE,
    PCM_FORMAT_MAX,
};

void set_audio_runtime_default();
int use_external_codec();
void set_output_device(int device);
uint8_t get_output_device();
void set_audio_mute(bool mute);
/* Mixer functions */
struct mixer *mixer_open(const char *device);
void mixer_close(struct mixer *mixer);
void mixer_dump(struct mixer *mixer);
struct mixer_ctl *get_ctl(struct mixer *mixer, char *name);
struct mixer_ctl *mixer_get_control(struct mixer *mixer, const char *name,
                                    unsigned index);
struct mixer_ctl *mixer_get_nth_control(struct mixer *mixer, unsigned n);
int mixer_ctl_set_value(struct mixer_ctl *ctl, int count, int val);

/* PCM functions */
struct pcm *pcm_open(unsigned flags, char *device);
int pcm_close(struct pcm *pcm);
int set_params(struct pcm *pcm, int path);

/* OpenQTI audio setting helpers */
int set_mixer_ctl(struct mixer *mixer, char *name, int value);
int stop_audio();
int start_audio(int type);
int dump_audio_mixer();
void handle_call_pkt(uint8_t *pkt, int from, int sz);
int set_audio_defaults();
int set_external_codec_defaults();
void set_auxpcm_sampling_rate(uint8_t mode);
void configure_custom_alert_tone(bool en);
int pcm_write(struct pcm *pcm, void *data, unsigned count);
unsigned int pcm_get_buffer_size(const struct pcm *pcm);
unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames);
#endif