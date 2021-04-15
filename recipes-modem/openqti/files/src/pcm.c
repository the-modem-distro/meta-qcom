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

  unsigned long periodSize, bufferSize, reqBuffSize;
  unsigned int periodTime, bufferTime;
  unsigned int requestedRate = pcm->rate;

  params =
      (struct snd_pcm_hw_params *)calloc(1, sizeof(struct snd_pcm_hw_params));
  if (!params) {
    fprintf(stderr, "failed to allocate ALSA hardware parameters!");
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
    fprintf(stderr, "cannot set hw params");
    return -1;
  }

  pcm->buffer_size = pcm_buffer_size(params);
  pcm->period_size = pcm_period_size(params);
  pcm->period_cnt = pcm->buffer_size / pcm->period_size;
  sparams =
      (struct snd_pcm_sw_params *)calloc(1, sizeof(struct snd_pcm_sw_params));
  if (!sparams) {
    fprintf(stderr, "failed to allocate ALSA software parameters!\n");
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
    fprintf(stderr, "cannot set sw params");
    return -1;
  }
  return 0;
}

/*
static int enable_timer(struct pcm *pcm) {

    pcm->timer_fd = open("/dev/snd/timer", O_RDWR | O_NONBLOCK);
    if (pcm->timer_fd < 0) {
       close(pcm->fd);
       printf("cannot open timer device 'timer'");
       return -1;
    }
    int arg = 1;
    struct snd_timer_params timer_param;
    struct snd_timer_select sel;
    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_TREAD, &arg) < 0) {
           printf("extended read is not supported (SNDRV_TIMER_IOCTL_TREAD)\n");
    }
    memset(&sel, 0, sizeof(sel));
    sel.id.dev_class = SNDRV_TIMER_CLASS_PCM;
    sel.id.dev_sclass = SNDRV_TIMER_SCLASS_NONE;
    sel.id.card = pcm->card_no;
    sel.id.device = pcm->device_no;
    if (pcm->flags & PCM_IN)
        sel.id.subdevice = 1;
    else
        sel.id.subdevice = 0;

    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_SELECT, &sel) < 0) {
          printf("SNDRV_TIMER_IOCTL_SELECT failed.\n");
          close(pcm->timer_fd);
          close(pcm->fd);
          return -1;
    }
    memset(&timer_param, 0, sizeof(struct snd_timer_params));
    timer_param.flags |= SNDRV_TIMER_PSFLG_AUTO;
    timer_param.ticks = 1;
    timer_param.filter = (1<<SNDRV_TIMER_EVENT_MSUSPEND) |
(1<<SNDRV_TIMER_EVENT_MRESUME) | (1<<SNDRV_TIMER_EVENT_TICK);

    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_PARAMS, &timer_param)< 0) {
           printf("SNDRV_TIMER_IOCTL_PARAMS failed\n");
    }
    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_START) < 0) {
           close(pcm->timer_fd);
           printf("SNDRV_TIMER_IOCTL_START failed\n");
    }
    return 0;
}

static int disable_timer(struct pcm *pcm) {
     if (pcm == -1)
         return 0;
     if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_STOP) < 0)
         printf("SNDRV_TIMER_IOCTL_STOP failed\n");
     return close(pcm->timer_fd);
}*/

int pcm_close(struct pcm *pcm) {
  if (pcm == NULL)
    return 0;
  /*
      if (pcm->flags & PCM_MMAP) {
          disable_timer(pcm);
          if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP) < 0) {
              printf("Reset failed\n");
          }

          if (munmap(pcm->addr, pcm->buffer_size))
              printf("munmap failed\n");

          if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_FREE) < 0) {
              printf("HW_FREE failed\n");
          }
      }*/

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
  struct snd_pcm_hw_params params;
  struct snd_pcm_sw_params sparams;
  unsigned period_sz;
  unsigned period_cnt;
  char *tmp;

  pcm = calloc(1, sizeof(struct pcm));
  if (!pcm)
    return NULL;

  strncpy(dname, device, sizeof(dname));

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
    printf("cannot open device '%s', errno %d", dname, errno);
    return NULL;
  }

  if (fcntl(pcm->fd, F_SETFL, fcntl(pcm->fd, F_GETFL) & ~O_NONBLOCK) < 0) {
    close(pcm->fd);
    free(pcm->sync_ptr);
    free(pcm);
    printf("failed to change the flag, errno %d", errno);
    return NULL;
  }
  /*
      if (pcm->flags & PCM_MMAP)
          enable_timer(pcm);*/

  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
    printf("cannot get info - %s", dname);
  }

  return pcm;
}
