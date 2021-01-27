#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sound/asound.h>

#define RXCTL "SEC_AUX_PCM_RX_Voice Mixer VoLTE"
#define TXCTL "VoLTE_Tx Mixer SEC_AUX_PCM_TX_VoLTE"
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
} suffixes[] = {
        {" Playback Volume", CTL_PLAYBACK_VOLUME},
        {" Capture Volume", CTL_CAPTURE_VOLUME},
        {" Volume", CTL_GLOBAL_VOLUME},
        {NULL, 0}
};


struct mixer *mixer_open(const char *device);
void mixer_close(struct mixer *mixer);
void mixer_dump(struct mixer *mixer);
struct mixer_ctl *get_ctl(struct mixer *mixer, char *name);
struct mixer_ctl *mixer_get_control(struct mixer *mixer,
                                    const char *name, unsigned index);
struct mixer_ctl *mixer_get_nth_control(struct mixer *mixer, unsigned n);
int mixer_ctl_set_value(struct mixer_ctl *ctl, int count, int val);

#endif