/* SPDX-License-Identifier: MIT */

#ifndef _IPCSECURITY_H_
#define _IPCSECURITY_H_
#include <stdint.h>
#include <sys/types.h>

struct irsc_rule {
  int rl_no;
  uint32_t service;
  uint32_t instance;
  unsigned dummy;
  gid_t group_id[0];
};

#define IOCTL_RULES _IOR(0xC3, 5, struct irsc_rule)
#define IRSC_INSTANCE_ALL 4294967295
#endif