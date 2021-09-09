/* SPDX-License-Identifier: MIT */

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdbool.h>
#include <stdint.h>

#define PERSIST_ADB_ON_MAGIC "persistent_adb_on"
#define PERSIST_ADB_OFF_MAGIC "persistent_adb_off"
#define PERSIST_USB_AUD_MAGIC "persistent_usbaudio_on"

int write_to(const char *path, const char *val, int flags);
uint32_t get_curr_timestamp();
void set_next_fastboot_mode(int flag);
void store_adb_setting(bool en);
void switch_adb(bool en);
int is_adb_enabled();
int get_audio_mode();
void store_audio_output_mode(uint8_t mode);
void reset_usb_port();
void restart_usb_stack();
void enable_usb_port();

int get_usb_state();
int set_smd_dtr();
void *gps_proxy();
void *rmnet_proxy(void *node_data);
void *dtr_monitor();
#endif