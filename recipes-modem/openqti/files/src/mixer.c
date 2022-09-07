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

/** Pieces of alsa_mixer.c **/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sound/asound.h>
#include <sound/tlv.h>
#include <sys/ioctl.h>

#include "../inc/audio.h"
#include "../inc/logger.h"

/* .5 for rounding before casting to non-decmal value */
/* Should not be used if you need decmal values */
/* or are expecting negitive indexes */
#define percent_to_index(val, min, max)                                        \
  ((val) * ((max) - (min)) * 0.01 + (min) + .5)

struct mixer *mixer_open(const char *device) {
  struct snd_ctl_elem_list elist;
  struct snd_ctl_elem_info tmp;
  struct snd_ctl_elem_id *eid = NULL;
  struct mixer *mixer = NULL;
  unsigned n, m;
  int fd;

  fd = open(device, O_RDWR);
  if (fd < 0) {
    logger(MSG_WARN, "Control open failed\n");
    return 0;
  }

  memset(&elist, 0, sizeof(elist));
  if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0) {
    logger(MSG_WARN, "SNDRV_CTL_IOCTL_ELEM_LIST failed\n");
    goto fail;
  }

  mixer = calloc(1, sizeof(*mixer));
  if (!mixer)
    goto fail;

  mixer->ctl = calloc(elist.count, sizeof(struct mixer_ctl));
  mixer->info = calloc(elist.count, sizeof(struct snd_ctl_elem_info));
  if (!mixer->ctl || !mixer->info)
    goto fail;

  eid = calloc(elist.count, sizeof(struct snd_ctl_elem_id));
  if (!eid)
    goto fail;

  mixer->count = elist.count;
  mixer->fd = fd;
  elist.space = mixer->count;
  elist.pids = eid;
  if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0)
    goto fail;

  for (n = 0; n < mixer->count; n++) {
    struct snd_ctl_elem_info *ei = mixer->info + n;
    ei->id.numid = eid[n].numid;
    if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, ei) < 0)
      goto fail;
    mixer->ctl[n].info = ei;
    mixer->ctl[n].mixer = mixer;
    if (ei->type == SNDRV_CTL_ELEM_TYPE_ENUMERATED) {
      char **enames = calloc(ei->value.enumerated.items, sizeof(char *));
      if (!enames)
        goto fail;
      mixer->ctl[n].ename = enames;
      for (m = 0; m < ei->value.enumerated.items; m++) {
        memset(&tmp, 0, sizeof(tmp));
        tmp.id.numid = ei->id.numid;
        tmp.value.enumerated.item = m;
        if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, &tmp) < 0)
          goto fail;
        enames[m] = strdup(tmp.value.enumerated.name);
        if (!enames[m])
          goto fail;
      }
    }
  }

  free(eid);
  return mixer;

fail:
  if (eid)
    free(eid);
  if (mixer)
    mixer_close(mixer);
  else if (fd >= 0)
    close(fd);
  return 0;
}

void mixer_close(struct mixer *mixer) {
  unsigned n, m;

  if (mixer->fd >= 0)
    close(mixer->fd);

  if (mixer->ctl) {
    for (n = 0; n < mixer->count; n++) {
      if (mixer->ctl[n].ename) {
        unsigned max = mixer->ctl[n].info->value.enumerated.items;
        for (m = 0; m < max; m++)
          free(mixer->ctl[n].ename[m]);
        free(mixer->ctl[n].ename);
      }
    }
    free(mixer->ctl);
  }

  if (mixer->info)
    free(mixer->info);

  free(mixer);
}

struct mixer_ctl *mixer_get_control(struct mixer *mixer, const char *name,
                                    unsigned index) {
  unsigned n;
  for (n = 0; n < mixer->count; n++) {
    if (mixer->info[n].id.index == index) {
      if (!strncmp(name, (char *)mixer->info[n].id.name,
                   sizeof(mixer->info[n].id.name))) {
        return mixer->ctl + n;
      }
    }
  }
  logger(MSG_ERROR, "%s: Mixer control %s not found\n", __func__, name);
  return 0;
}

struct mixer_ctl *mixer_get_nth_control(struct mixer *mixer, unsigned n) {
  if (n < mixer->count)
    return mixer->ctl + n;
  return 0;
}

struct mixer_ctl *get_ctl(struct mixer *mixer, char *name) {
  char *p;
  unsigned idx = 0;
  if (isdigit(name[0]))
    return mixer_get_nth_control(mixer, atoi(name) - 1);

  p = strrchr(name, '#');
  if (p) {
    *p++ = 0;
    idx = atoi(p);
  }

  return mixer_get_control(mixer, name, idx);
}

unsigned get_ctl_index(struct mixer *mixer, char *name) {
  char *p;
  unsigned idx = 0;
  if (isdigit(name[0]))
    return idx;

  p = strrchr(name, '#');
  if (p) {
    *p++ = 0;
    idx = atoi(p);
  }

  return idx;
}

int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value) {
  int i;
  struct snd_ctl_elem_value ev;
  if (!ctl) {
    logger(MSG_ERROR, "%s: Invalid mixer_ctl\n", __func__);
    return -EINVAL;
  }

  memset(&ev, 0, sizeof(ev));
  ev.id.numid = ctl->info->id.numid;
  switch (ctl->info->type) {
  case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
    logger(MSG_DEBUG, "%s: Mixer control type: Boolean\n", __func__);
    for (i = 0; i < ctl->info->count; i++)
      ev.value.integer.value[i] = !!value;
    break;

  case SNDRV_CTL_ELEM_TYPE_INTEGER:
    logger(MSG_DEBUG, "%s: Mixer control type: Integer\n", __func__);
    for (i = 0; i < ctl->info->count; i++)
      ev.value.integer.value[i] = value;
    break;

  case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
    logger(MSG_DEBUG, "%s: Mixer control type: Enum\n", __func__);
    for (i = 0; i < ctl->info->count; i++)
      ev.value.enumerated.item[i] = value;
    break;

  case SNDRV_CTL_ELEM_TYPE_BYTES:
    logger(MSG_DEBUG, "%s: Mixer control type: Bytes\n", __func__);
    for (i = 0; i < ctl->info->count; i++)
      ev.value.bytes.data[i] = value;
    break;

  default:
    logger(MSG_ERROR, "%s: Invalid mixer control type\n", __func__);
    return -EINVAL;
  }
  return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);
}

int mixer_ctl_set_gain(struct mixer_ctl *ctl, int call_type, int value) {

  struct snd_ctl_elem_value ev;
  uint32_t session_id;
  if (!ctl) {
    logger(MSG_ERROR, "%s: Invalid mixer_ctl\n", __func__);
    return -EINVAL;
  }

  memset(&ev, 0, sizeof(ev));
  ev.id.numid = ctl->info->id.numid;
  switch (call_type) {
  case 1:
    session_id = VOICE_SESSION_VSID;
    break;
  case 2:
    session_id = VOLTE_SESSION_VSID;
    break;
  default:
    session_id = VOICE_SESSION_VSID;
    break;
  }
  switch (ctl->info->type) {
  case SNDRV_CTL_ELEM_TYPE_INTEGER:
    logger(MSG_DEBUG, "%s: Mixer control type: Integer\n", __func__);
    ev.value.integer.value[0] = value; // Volume
    ev.value.integer.value[1] = session_id;
    ev.value.integer.value[2] = 0; // Ramp duration
    break;
  default:
    logger(MSG_ERROR, "%s: Invalid mixer control type\n", __func__);
    return -EINVAL;
  }
  return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);
}
