/*
** Copyright 2010, The Android Open-Source Project
** Copyright (c) 2011-2013, 2015 The Linux Foundation. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/** Pieces of alsa_pcm.c **/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/audio.h"
#include "../inc/logger.h"

static inline int param_is_mask(int p) {
  return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
         (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p) {
  return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
         (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *
param_to_interval(struct snd_pcm_hw_params *p, int n) {
  return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p,
                                             int n) {
  return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}
int param_set_sw_params(struct pcm *pcm, struct snd_pcm_sw_params *sparams) {
  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, sparams)) {
    return -EPERM;
  }
  pcm->sw_p = sparams;
  return 0;
}

int pcm_buffer_size(struct snd_pcm_hw_params *params) {
  struct snd_interval *i =
      param_to_interval(params, SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
  return i->min;
}

int pcm_period_size(struct snd_pcm_hw_params *params) {
  struct snd_interval *i =
      param_to_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES);
  return i->min;
}

void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit) {
  if (bit >= SNDRV_MASK_MAX)
    return;
  if (param_is_mask(n)) {
    struct snd_mask *m = param_to_mask(p, n);
    m->bits[0] = 0;
    m->bits[1] = 0;
    m->bits[bit >> 5] |= (1 << (bit & 31));
  }
}

void param_init(struct snd_pcm_hw_params *p) {
  int n;
  memset(p, 0, sizeof(*p));
  for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK; n <= SNDRV_PCM_HW_PARAM_LAST_MASK;
       n++) {
    struct snd_mask *m = param_to_mask(p, n);
    m->bits[0] = ~0;
    m->bits[1] = ~0;
  }
  for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
       n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = 0;
    i->max = ~0;
  }
}

void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned val) {
  if (param_is_interval(n)) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
  }
}

void param_set_max(struct snd_pcm_hw_params *p, int n, unsigned val) {
  if (param_is_interval(n)) {
    struct snd_interval *i = param_to_interval(p, n);
    i->max = val;
  }
}

void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned val) {
  if (param_is_interval(n)) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
    i->max = val;
    i->integer = 1;
  }
}

int param_set_hw_refine(struct pcm *pcm, struct snd_pcm_hw_params *params) {
  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_REFINE, params)) {
    return -EPERM;
  }
  return 0;
}

int param_set_hw_params(struct pcm *pcm, struct snd_pcm_hw_params *params) {
  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, params)) {
    return -EPERM;
  }
  pcm->hw_p = params;
  return 0;
}

int set_params(struct pcm *pcm, int path) {
  struct snd_pcm_hw_params *params;
  struct snd_pcm_sw_params *sparams;

  params =
      (struct snd_pcm_hw_params *)calloc(1, sizeof(struct snd_pcm_hw_params));
  if (!params) {
    logger(MSG_ERROR, "%s: failed to allocate ALSA hardware parameters!", __func__);
    return -ENOMEM;
  }

  param_init(params);

  param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                 (pcm->flags & PCM_MMAP) ? SNDRV_PCM_ACCESS_MMAP_INTERLEAVED
                                         : SNDRV_PCM_ACCESS_RW_INTERLEAVED);
  param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT, SNDRV_PCM_FORMAT_S16_LE);
  param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);
  param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 1000);
  param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
  param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                pcm->channels - 1 ? 32 : 16);
  param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS, pcm->channels);
  param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, pcm->rate);

  param_set_hw_refine(pcm, params);
  if (param_set_hw_params(pcm, params)) {
    logger(MSG_ERROR, "%s: cannot set hw params\n", __func__);
    return -1;
  }

  pcm->buffer_size = pcm_buffer_size(params);
  pcm->period_size = pcm_period_size(params);
  pcm->period_cnt = pcm->buffer_size / pcm->period_size;
  sparams =
      (struct snd_pcm_sw_params *)calloc(1, sizeof(struct snd_pcm_sw_params));
  if (!sparams) {
    logger(MSG_ERROR, "%s: failed to allocate ALSA software parameters!\n", __func__);
    return -ENOMEM;
  }
  sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
  sparams->period_step = 1;
  sparams->avail_min =
      (pcm->flags & PCM_MONO) ? pcm->period_size / 2 : pcm->period_size / 4;

  /* start after at least two periods are prefilled */
  if (path == PCM_IN)
    sparams->start_threshold =
        (pcm->flags & PCM_MONO) ? pcm->period_size : pcm->period_size / 2;
  else
    sparams->start_threshold = 1;

  sparams->stop_threshold =
      (pcm->flags & PCM_MONO) ? pcm->buffer_size / 2 : pcm->buffer_size / 4;
  sparams->xfer_align = (pcm->flags & PCM_MONO)
                            ? pcm->period_size / 2
                            : pcm->period_size / 4; /* needed for old kernels */
  sparams->silence_size = 0;
  sparams->silence_threshold = 0;

  if (param_set_sw_params(pcm, sparams)) {
    logger(MSG_ERROR, "%s: cannot set sw params", __func__);
    return -1;
  }
  return 0;
}

int pcm_close(struct pcm *pcm) {
  if (pcm == NULL)
    return 0;

  if (pcm->fd >= 0)
    close(pcm->fd);
  pcm->running = 0;
  pcm->buffer_size = 0;
  pcm->fd = -1;
  if (pcm->sw_p)
    free(pcm->sw_p);
  if (pcm->hw_p)
    free(pcm->hw_p);
  if (pcm->sync_ptr)
    free(pcm->sync_ptr);
  free(pcm);
  return 0;
}

struct pcm *pcm_open(unsigned flags, char *device) {
  char dname[19];
  struct pcm *pcm;
  struct snd_pcm_info info;

  pcm = calloc(1, sizeof(struct pcm));
  if (!pcm)
    return NULL;

  strncpy(dname, device, 18);

  if (flags & PCM_IN) {
    strncat(dname, "c", (sizeof("c") + strlen(dname)));
  } else {
    strncat(dname, "p", (sizeof("p") + strlen(dname)));
  }

  pcm->sync_ptr = calloc(1, sizeof(struct snd_pcm_sync_ptr));
  if (!pcm->sync_ptr) {
    free(pcm);
    return NULL;
  }
  pcm->flags = flags;

  pcm->fd = open(dname, O_RDWR | O_NONBLOCK);
  if (pcm->fd < 0) {
    free(pcm->sync_ptr);
    free(pcm);
    logger(MSG_ERROR, "cannot open device '%s', errno %d", dname, errno);
    return NULL;
  }

  if (fcntl(pcm->fd, F_SETFL, fcntl(pcm->fd, F_GETFL) & ~O_NONBLOCK) < 0) {
    close(pcm->fd);
    free(pcm->sync_ptr);
    free(pcm);
    logger(MSG_ERROR, "failed to change the flag, errno %d", errno);
    return NULL;
  }
  /*
      if (pcm->flags & PCM_MMAP)
          enable_timer(pcm);*/

  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
    logger(MSG_ERROR, "cannot get info - %s", dname);
  }

  return pcm;
}

int sync_ptr(struct pcm *pcm) {
  int err;
  err = ioctl(pcm->fd, SNDRV_PCM_IOCTL_SYNC_PTR, pcm->sync_ptr);
  if (err < 0) {
    err = errno;
    logger(MSG_ERROR, "SNDRV_PCM_IOCTL_SYNC_PTR failed %d \n", err);
    return err;
  }
  return 0;
}

int pcm_prepare(struct pcm *pcm) {
  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE)) {
    logger(MSG_ERROR, "cannot prepare channel: errno =%d\n", -errno);
    return -errno;
  }
  pcm->running = 1;
  return 0;
}

static int pcm_write_nmmap(struct pcm *pcm, void *data, unsigned count) {
  struct snd_xferi x;
  int channels =
      (pcm->flags & PCM_MONO) ? 1 : ((pcm->flags & PCM_5POINT1) ? 6 : 2);
  if (pcm->flags & PCM_IN)
    return -EINVAL;
  x.buf = data;
  x.frames = (count / (channels * 2));
  for (;;) {
    if (!pcm->running) {
      if (pcm_prepare(pcm))
        return -errno;
    }
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x)) {
      if (errno == EPIPE) {
        /* we failed to make our window -- try to restart */
        logger(MSG_DEBUG, "Buffer Underrun Error\n");
        pcm->underruns++;
        pcm->running = 0;
        continue;
      }
      return -errno;
    }
    if (pcm->flags & DEBUG_ON)
      logger(MSG_ERROR, "Sent frame\n");
    return 0;
  }
}
int pcm_write(struct pcm *pcm, void *data, unsigned count) {
  return pcm_write_nmmap(pcm, data, count);
}

/** Gets the buffer size of the PCM.
 * @param pcm A PCM handle.
 * @return The buffer size of the PCM.
 * @ingroup libtinyalsa-pcm
 */
unsigned int pcm_get_buffer_size(const struct pcm *pcm) {
  return pcm->buffer_size;
}
static unsigned int pcm_format_to_bits(enum pcm_format format) {
  switch (format) {
  case PCM_FORMAT_S32_LE:
    return 32;
  default:
  case PCM_FORMAT_S16_LE:
    return 16;
  };
}
unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames) {
  return frames * pcm->channels * (pcm_format_to_bits(pcm->format) >> 3);
}

/** Determines how many frames of a PCM can fit into a number of bytes.
 * @param pcm A PCM handle.
 * @param bytes The number of bytes.
 * @return The number of frames that may fit into @p bytes
 * @ingroup libtinyalsa-pcm
 */
unsigned int pcm_bytes_to_frames(const struct pcm *pcm, unsigned int bytes)
{
    return bytes / (pcm->channels *
        (pcm_format_to_bits(pcm->format) >> 3));
}

int pcm_read(struct pcm *pcm, void *data, uint32_t frames)
{
    struct snd_xferi x;
    if (!(pcm->flags & PCM_IN))
        return -EINVAL;
    x.buf = data;
    x.frames = frames;
    for (;;) {
        if (!pcm->running) {
            if (pcm_prepare(pcm))
                return -errno;
            if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_START)) {
                logger(MSG_ERROR, "%s: SNDRV_PCM_IOCTL_START failed\n", __func__);
                return -errno;
            }
            pcm->running = 1;
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x)) {
            if (errno == EPIPE) {
                /* we failed to make our window -- try to restart */
                logger(MSG_ERROR, "%s: Overrun Error\n", __func__);
                pcm->underruns++;
                pcm->running = 0;
                continue;
            }
            logger(MSG_ERROR, "%s: Error %d\n", __func__, errno);
            return -errno;
        }
        return 0;
    }
}