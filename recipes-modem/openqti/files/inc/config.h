/* SPDX-License-Identifier: MIT */

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define PERSIST_CUSTOM_ALERT_TONE "cust_alert_tone"
#define CONFIG_FILE_PATH "/persist/openqti.conf"
#define SCHEDULER_DATA_FILE_PATH "/persist/sched.raw"
#define PERSISTENT_LOGFILE_PATH "/persist/log"
#define MAX_NAME_SZ 32
struct config_prototype {
  uint8_t custom_alert_tone;
  uint8_t persistent_logging;
  char user_name[MAX_NAME_SZ];
  char modem_name[MAX_NAME_SZ];
  uint8_t signal_tracking;
  uint8_t signal_tracking_mode; 
  uint8_t sms_logging;
  uint8_t callwait_autohangup;
  uint8_t automatic_call_recording;
  uint8_t allow_internal_modem_connectivity;
  bool first_boot;

};
/* 
signal_tracking_operating mode (requires signal_tracking ON and PERSIST)
0: Standalone + info (learning mode)
1: Standalone + hardened (autoreject)
2: Standalone + OpenCellid: Info (learning mode)
3: Standalone + OpenCellID + Hardened (autoreject)
*/

/* Set mount as readwrite or readonly */
int set_persistent_partition_rw();
int set_persistent_partition_ro();
void do_sync_fs();

int set_initial_config();
int read_settings_from_file();

/* Signal tracking */
int is_signal_tracking_enabled();
uint8_t get_signal_tracking_mode();
void enable_signal_tracking(bool en);
void set_signal_tracking_mode(uint8_t mode);

/* Custom alert tone */
int use_custom_alert_tone();
void set_custom_alert_tone(bool en);

/* Modem Name */
int get_modem_name(char *buff);
void set_modem_name(char *name);
/* User name */
int get_user_name(char *buff);
void set_user_name(char *name);

/* Persistent logging */
int use_persistent_logging();
void set_persistent_logging(bool en);
char *get_openqti_logfile();

/* Is first boot? */
bool is_first_boot();
void clear_ifrst_boot_flag();

/* SMS logging */
int is_sms_logging_enabled();
void set_sms_logging(bool en);

/* Automatically hang up call waiting */
int callwait_auto_hangup_operation_mode();
void enable_call_waiting_autohangup(uint8_t en);

/* Automatic call recording */
int is_automatic_call_recording_enabled();
void set_automatic_call_recording(uint8_t mode);

/* SMS logging */
int is_internal_connect_enabled();
void set_internal_connectivity(bool en);


#endif