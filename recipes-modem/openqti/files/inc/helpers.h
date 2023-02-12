/* SPDX-License-Identifier: MIT */

#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define PERSIST_ADB_ON_MAGIC "persistent_adb_on"
#define PERSIST_ADB_OFF_MAGIC "persistent_adb_off"
#define PERSIST_USB_AUD_MAGIC "persistent_usbaudio_on"

#define INPUT_DEV "/dev/input/event0"

#define MSG_DELETE_PARTIAL_CMD "AT+CMGD="

#define CPUFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPUFREQ_PERF "performance"
#define CPUFREQ_PS "powersave"

int write_to(const char *path, const char *val, int flags);
uint32_t get_curr_timestamp();
void store_adb_setting(bool en);
void switch_adb(bool en);
int is_adb_enabled();
int get_audio_mode();
void store_audio_output_mode(uint8_t mode);

void reset_usb_port();
void restart_usb_stack();
void enable_usb_port();
void set_suspend_inhibit(bool mode);
void prepare_dtr_gpio();
uint8_t get_dtr_state();
uint8_t pulse_ring_in();

int get_int_from_str(char *str, int offset);
void *power_key_event();
int read_adsp_version();
int wipe_message_storage();
void add_message_to_queue(uint8_t *message, size_t len);
int send_at_command(char *at_command, size_t cmdlen, char *response, size_t response_sz);
void enable_cpufreq_performance_mode(bool enable);
#endif