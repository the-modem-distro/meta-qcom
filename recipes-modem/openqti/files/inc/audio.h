/* SPDX-License-Identifier: MIT */

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sound/asound.h>
#include <stdint.h>
#include <stdbool.h>

/* VoLTE calls use these mixers */
#define RXCTL_VOLTE "SEC_AUX_PCM_RX_Voice Mixer VoLTE"
#define TXCTL_VOLTE "VoLTE_Tx Mixer SEC_AUX_PCM_TX_VoLTE"

/* Keeping these as reference but not using them since we're not using ACDBs */
#define AFECTL_VOLTE "SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX"
#define AFERX_VOLTE "AFE_PCM_RX_Voice Mixer VoLTE"
#define AFETX_VOLTE "VoLTE_Tx Mixer AFE_PCM_TX_VoLTE"

/* Normal Voice calls */
#define RXCTL_VOICE "SEC_AUX_PCM_RX_Voice Mixer CSVoice"
#define TXCTL_VOICE "Voice_Tx Mixer SEC_AUX_PCM_TX_Voice"

/* Keeping these as reference but not using them since we're not using ACDBs */
#define AFECTL_VOICE "SEC_AUXPCM_RX Port Mixer SEC_AUX_PCM_UL_TX"
#define AFERX_VOICE "AFE_PCM_RX_Voice Mixer CSVoice"
#define AFETX_VOICE "Voice_Tx Mixer AFE_PCM_TX_Voice"

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
  unsigned running : 1;
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
void set_auxpcm_sampling_rate(uint8_t mode);

#endif