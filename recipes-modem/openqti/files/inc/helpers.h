/* SPDX-License-Identifier: MIT */

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdbool.h>
#include <stdint.h>

int write_to(const char *path, const char *val, int flags);
uint32_t get_curr_timestamp();
void set_next_fastboot_mode(int flag);
void store_adb_setting(bool en);
void switch_adb(bool en);
int is_adb_enabled();

void *gps_proxy();
void *rmnet_proxy(void *node_data);
#endif