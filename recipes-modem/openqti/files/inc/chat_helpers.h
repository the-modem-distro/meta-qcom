/* SPDX-License-Identifier: MIT */

#ifndef __CHAT_HELPERS_H__
#define __CHAT_HELPERS_H__
#include "helpers.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define DICT_PATH "/opt/openqti/dict.txt"
#define CHAT_CMD_HELP 8

/* Command IDs */
enum {
  CMD_UNKNOWN = -1,
  CMD_ID_GET_NAME = 0,
  CMD_ID_GET_UPTIME,
  CMD_ID_GET_LOADAVG,
  CMD_ID_GET_VERSION,
  CMD_ID_GET_USB_SUSPEND_STATE,
  CMD_ID_GET_MEM,
  CMD_ID_GET_NET_STATS,
  CMD_ID_GET_GPS_STATS,
  CMD_ID_GET_HELP,
  CMD_ID_SET_USB_SUSPEND_BLOCK,
  CMD_ID_SET_USB_SUSPEND_ALLOW,
  CMD_ID_SET_ADB_ON,
  CMD_ID_SET_ADB_OFF,
  CMD_ID_GET_HISTORY,
  CMD_ID_GET_LOG,
  CMD_ID_GET_KERNEL_LOG,
  CMD_ID_GET_RECONNECTS,
  CMD_ID_ACTION_CALL_OWNER,
  CMD_ID_GET_OWNER_NAME,
  CMD_ID_ACTION_POWEROFF,
  CMD_ID_GET_GSM_SIGNAL,
  CMD_ID_ACTION_REBOOT,
  CMD_ID_GET_NET_REPORT,
  CMD_ID_ACTION_ENABLE_SIGNAL_TRACKING,
  CMD_ID_ACTION_DISABLE_SIGNAL_TRACKING,
  CMD_ID_ACTION_ENABLE_PERSISTENT_LOGGING,
  CMD_ID_ACTION_DISABLE_PERSISTENT_LOGGING,
  CMD_ID_ACTION_ENABLE_SMS_LOGGING,
  CMD_ID_ACTION_DISABLE_SMS_LOGGING,
  CMD_ID_UTIL_LIST_ACTIVE_TASKS,
  CMD_ID_GET_FW_TY,
  CMD_ID_ACTION_ENABLE_CELL_BROADCAST,
  CMD_ID_ACTION_DISABLE_CELL_BROADCAST,
  CMD_ID_ACTION_CALLWAIT_AUTO_HANGUP,
  CMD_ID_ACTION_CALLWAIT_AUTO_IGNORE,
  CMD_ID_ACTION_CALLWAIT_MODE_DEFAULT,
  CMD_ID_ACTION_ENABLE_CAT,
  CMD_ID_ACTION_DISABLE_CAT,
  CMD_ID_ACTION_RECORD_CALLS_AND_RECYCLE,
  CMD_ID_ACTION_RECORD_CALLS_ALL,
  CMD_ID_ACTION_RECORD_CALLS_NONE,
  CMD_ID_ACTION_RECORD_CALL_CURRENT,
  CMD_ID_ACTION_RECORD_CALL_NEXT,
  CMD_ID_ACTION_RECORD_CALL_CANCEL_TASK,
  CMD_ID_DEBUG_GET_MM_SAMPLE_GSM7_CB_MSG,
  CMD_ID_DEBUG_GET_MM_SAMPLE_UCS2_CB_MSG,
  CMD_ID_ACTION_ENABLE_INTERNAL_NETWORKING,
  CMD_ID_ACTION_DISABLE_INTERNAL_NETWORKING,
  CMD_ID_ACTION_DISABLE_DO_NOT_DISTURB,
  CMD_ID_ACTION_DEBUG_DISABLE_SVC,
  CMD_ID_GET_KNOWN_QMI_SERVICES,
  CMD_ID_ACTION_DEBUG_ENABLE_NETWORK_DATA_DUMP,
  CMD_ID_ACTION_DEBUG_DISABLE_NETWORK_DATA_DUMP,
  CMD_ID_ACTION_ENABLE_ST_NETWORK_DOWNGRADE_NOTIF,
  CMD_ID_ACTION_DISABLE_ST_NETWORK_DOWNGRADE_NOTIF,
  CMD_ID_ACTION_INTERNAL_NETWORK_CLEAR_AUTH_DATA,
  CMD_ID_ACTION_MESSAGE_RECOVERY_ENABLE,
  CMD_ID_ACTION_MESSAGE_RECOVERY_DISABLE,
  CMD_ID_ACTION_INTERNAL_NETWORK_START,
  CMD_ID_ACTION_INTERNAL_NETWORK_STOP,
  /* Previously called "partial commands" */
  CMD_ID_SET_MODEM_NAME,
  CMD_ID_SET_OWNER_NAME,
  CMD_ID_ACTION_CALL_OWNER_COUNTDOWN,
  CMD_ID_ACTION_SET_REMINDER,
  CMD_ID_ACTION_WAKE_OWNER,
  CMD_ID_ACTION_DELETE_TASK,
  CMD_ID_ACTION_ENABLE_DND,
  CMD_ID_DEBUG_ENABLE_SERVICE_LOGGING,
  CMD_ID_ACTION_SET_ST_MODE,
  CMD_ID_ACTION_SET_ST_NOTIFICATION_LEVEL,
  CMD_ID_DEFINE,
  CMD_ID_ACTION_INTERNAL_NETWORK_SET_APN,
  CMD_ID_ACTION_INTERNAL_NETWORK_SET_USER,
  CMD_ID_ACTION_INTERNAL_NETWORK_SET_PASS,
  CMD_ID_ACTION_INTERNAL_NETWORK_SET_AUTH_METHOD,
};

void get_rmnet_stats_cmd();
void get_gps_stats_cmd();
void cmd_get_help();

int get_uptime();
int cmd_get_load_avg();
int cmd_get_memory();
void set_custom_modem_name(uint8_t *command);
void enable_service_debugging_for_service_id(uint8_t *command);
void set_new_signal_tracking_mode(uint8_t *command);
void set_custom_user_name(uint8_t *command);
void delete_task(uint8_t *command);
void debug_gsm7_cb_message(uint8_t *command);
void debug_ucs2_cb_message(uint8_t *command);
void dump_signal_report();
void *delayed_shutdown();
void *delayed_reboot();
void *schedule_call(void *cmd);
void render_gsm_signal_data();
void schedule_reminder(uint8_t *command);
void schedule_wakeup(uint8_t *command);
void suspend_call_notifications(uint8_t *command);
int find_dictionary_entry(char *word);
void search_dictionary_entry(uint8_t *command);
void configure_new_apn(uint8_t *command);
void configure_apn_username(uint8_t *command);
void configure_apn_password(uint8_t *command);
void configure_internal_network_auth_method(uint8_t *command);
void configure_signal_tracking_cell_notification(uint8_t *command);
void clear_internal_networking_auth();
void set_cb_broadcast(bool en);

void cmd_get_openqti_log();
void cmd_get_kernel_log();
void cmd_get_sw_version();
void cmd_thank_you();
#endif
