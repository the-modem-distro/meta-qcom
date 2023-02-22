/* SPDX-License-Identifier: MIT */

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define PERSIST_CUSTOM_ALERT_TONE "cust_alert_tone"
#define CONFIG_FILE_PATH "/persist/openqti.conf"
#define SCHEDULER_DATA_FILE_PATH "/persist/sched.raw"
#define PERSISTENT_PATH "/persist/"
#define VOLATILE_PATH "/tmp/"
#define MAX_NAME_SZ 128
#define MAX_APN_FIELD_SZ 128
struct config_prototype {
  uint8_t custom_alert_tone;
  uint8_t persistent_logging;
  char user_name[MAX_NAME_SZ];
  char modem_name[MAX_NAME_SZ];
  uint8_t signal_tracking;
  uint8_t signal_tracking_mode;
  uint8_t signal_tracking_notify_downgrade;
  uint8_t signal_tracking_notify_cell_change; // no, only new, all
  uint8_t sms_logging;
  uint8_t callwait_autohangup;
  uint8_t automatic_call_recording;
  uint8_t dump_network_tables;
  bool first_boot;
  uint8_t allow_internal_modem_connectivity;
  uint8_t apn_auth_method; // 0 none, 1 PAP, 2 CHAP
  uint8_t apn_ip_family; // ipv4 - ipv6: not implemented
  char apn_addr[MAX_APN_FIELD_SZ];
  char apn_username[MAX_APN_FIELD_SZ];
  char apn_password[MAX_APN_FIELD_SZ];
  // IPv6 is not currently supported, but I'll have to look into it

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
uint8_t is_signal_tracking_enabled();
uint8_t get_signal_tracking_mode();
void enable_signal_tracking(bool en);
void set_signal_tracking_mode(uint8_t mode);

uint8_t is_signal_tracking_downgrade_notification_enabled();
uint8_t get_signal_tracking_cell_change_notification_mode();
void set_signal_tracking_downgrade_notification(uint8_t enable);
void set_signal_tracking_cell_change_notification(uint8_t mode);

/* Custom alert tone */
uint8_t use_custom_alert_tone();
void set_custom_alert_tone(bool en);

/* Modem Name */
uint8_t get_modem_name(char *buff);
void set_modem_name(char *name);
/* User name */
uint8_t get_user_name(char *buff);
void set_user_name(char *name);

/* Persistent logging */
uint8_t use_persistent_logging();
void set_persistent_logging(bool en);
char *get_openqti_logfile();
char *get_default_logpath();
/* Is first boot? */
bool is_first_boot();
void clear_ifrst_boot_flag();

/* SMS logging */
uint8_t is_sms_logging_enabled();
void set_sms_logging(bool en);

/* Automatically hang up call waiting */
uint8_t callwait_auto_hangup_operation_mode();
void enable_call_waiting_autohangup(uint8_t en);

/* Automatic call recording */
uint8_t is_automatic_call_recording_enabled();
void set_automatic_call_recording(uint8_t mode);

/* Internal Networking */
/* Getters */
uint8_t is_internal_connect_enabled();
char *get_internal_network_apn_name();
char *get_internal_network_username();
char *get_internal_network_pass();
uint8_t get_internal_network_auth_method();
/* Setters */
void set_internal_connectivity(bool en);
void set_internal_network_apn_name(char *apn);
void set_internal_network_username(char *username);
void set_internal_network_pass(char *pass);
void set_internal_network_auth_method(uint8_t method);

/* Automatically export cell location data as CSV */
uint8_t get_dump_network_tables_config();
void enable_dump_network_tables(bool en);
#endif