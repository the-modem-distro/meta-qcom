/* SPDX-License-Identifier: MIT */

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdint.h>

int write_to(const char *path, const char *val, int flags);

void *two_way_proxy(void *node_data);
void *rmnet_proxy(void *node_data);

#endif