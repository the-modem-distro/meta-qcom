/* SPDX-License-Identifier: MIT */

#ifndef _PROXY_H
#define _PROXY_H

#include <stdbool.h>
#include <stdint.h>

struct pkt_stats {
  uint32_t bypassed;
  uint32_t empty;
  uint32_t discarded;
  uint32_t allowed;
  uint32_t failed;
  uint32_t other;
};
void proxy_rt_reset();
void enable_service_debugging(uint8_t service_id);
void disable_service_debugging();

struct pkt_stats get_rmnet_stats();
struct pkt_stats get_gps_stats();
int get_transceiver_suspend_state();
void *gps_proxy();
void *rmnet_proxy(void *node_data);
#endif