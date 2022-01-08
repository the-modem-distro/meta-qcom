/* SPDX-License-Identifier: MIT */

#ifndef _PROXY_H
#define _PROXY_H

#include <stdbool.h>
#include <stdint.h>

int get_transceiver_suspend_state();
void *gps_proxy();
void *rmnet_proxy(void *node_data);
#endif