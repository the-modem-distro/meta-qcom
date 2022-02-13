/* SPDX-License-Identifier: MIT */

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdbool.h>
#include <stdint.h>

#define PERSIST_ADB_ON_MAGIC "persistent_adb_on"
#define PERSIST_ADB_OFF_MAGIC "persistent_adb_off"
#define PERSIST_USB_AUD_MAGIC "persistent_usbaudio_on"
#define PERSIST_CUSTOM_ALERT_TONE "cust_alert_tone"

int write_to(const char *path, const char *val, int flags);
uint32_t get_curr_timestamp();
void set_next_fastboot_mode(int flag);
void store_adb_setting(bool en);
void switch_adb(bool en);
int is_adb_enabled();
int get_audio_mode();
void store_audio_output_mode(uint8_t mode);
int use_custom_alert_tone();
void set_custom_alert_tone(bool en);
void reset_usb_port();
void restart_usb_stack();
void enable_usb_port();
void set_suspend_inhibit(bool mode);
void prepare_dtr_gpio();
uint8_t get_dtr_state();
uint8_t pulse_ring_in();
/* Modem Name */
int get_modem_name(char *buff);
int get_user_name(char *buff);
void set_modem_name(char *name);
void set_user_name(char *name);
#endif