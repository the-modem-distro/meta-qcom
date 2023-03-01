/* SPDX-License-Identifier: MIT */

#include "command.h"
#include "adspfw.h"
#include "audio.h"
#include "call.h"
#include "cell_broadcast.h"
#include "chat_helpers.h"
#include "config.h"
#include "dms.h"
#include "ipc.h"
#include "logger.h"
#include "nas.h"
#include "proxy.h"
#include "scheduler.h"
#include "sms.h"
#include "tracking.h"
#include "wds.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct {
  bool is_unlocked;
  uint32_t unlock_time;
  uint8_t cmd_history[1024];
  uint16_t cmd_position;
  uint32_t last_cmd_timestamp;
  char user_name[MAX_NAME_SZ];
  char bot_name[MAX_NAME_SZ];
} cmd_runtime;

char *get_rt_modem_name() { return cmd_runtime.bot_name; }

char *get_rt_user_name() { return cmd_runtime.user_name; }

size_t set_message_response(uint8_t command_id, uint8_t *buff) {
  size_t strsz = 0;
  strsz = snprintf((char *)buff, MAX_MESSAGE_SIZE, "%s\n",
                   bot_commands[command_id].cmd_text);
  return strsz;
}

void send_default_response(uint8_t cmd_id) {
  size_t strsz = 0;
  uint8_t reply[MAX_MESSAGE_SIZE];
  strsz = set_message_response(cmd_id, reply);
  add_message_to_queue(reply, strsz);
}

void add_to_history(uint8_t command_id) {
  if (cmd_runtime.cmd_position >= 1023) {
    cmd_runtime.cmd_position = 0;
  }
  cmd_runtime.cmd_history[cmd_runtime.cmd_position] = command_id;
  cmd_runtime.cmd_position++;
}

uint16_t find_cmd_history_match(uint8_t command_id) {
  uint16_t match = 0;
  uint16_t countdown;
  if (cmd_runtime.cmd_position > 5) {
    countdown = cmd_runtime.cmd_position - 5;
    for (uint16_t i = cmd_runtime.cmd_position; i >= countdown; i--) {
      if (cmd_runtime.cmd_history[i] == command_id) {
        match++;
      }
    }
  }
  return match;
}

void get_names() {
  get_modem_name(cmd_runtime.bot_name);
  get_user_name(cmd_runtime.user_name);
}

void set_cmd_runtime_defaults() {
  cmd_runtime.is_unlocked = false;
  cmd_runtime.unlock_time = 0;
  strncpy(cmd_runtime.user_name, "User",
          32); // FIXME: Allow user to set a custom name
  strncpy(cmd_runtime.bot_name, "Modem",
          32); // FIXME: Allow to change modem name
  get_names();
}

uint8_t parse_command(uint8_t *command) {
  int ret = 0;
  int cmd_id = -1;
  int strsz = 0;
  pthread_t disposable_thread;
  char lowercase_cmd[160];
  uint8_t reply[MAX_MESSAGE_SIZE] = {0};
  srand(time(NULL));

  for (uint8_t i = 0; i < command[i]; i++) {
    lowercase_cmd[i] = tolower(command[i]);
  }
  lowercase_cmd[strlen((char *)command)] = '\0';
  /* Static commands */
  for (uint8_t i = 0; i < (sizeof(bot_commands) / sizeof(bot_commands[0]));
       i++) {
    if (!bot_commands[i].is_partial) {

      if ((strcmp((char *)command, bot_commands[i].cmd) == 0) ||
          (strcmp(lowercase_cmd, bot_commands[i].cmd) == 0)) {
        cmd_id = bot_commands[i].id;
      }
    } else {
      if ((strstr((char *)command, bot_commands[i].cmd) != NULL) ||
          (strstr((char *)lowercase_cmd, bot_commands[i].cmd) != NULL)) {
        cmd_id = bot_commands[i].id;
      }
    }
  }

  switch (cmd_id) {
  case CMD_ID_GET_NAME:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.bot_name);
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_GET_UPTIME:
    cmd_get_uptime();
    break;
  case CMD_ID_GET_LOADAVG:
    cmd_get_load_avg();
    break;
  case CMD_ID_GET_VERSION:
    cmd_get_sw_version();
    break;
  case CMD_ID_GET_USB_SUSPEND_STATE:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "USB Suspend state: %i\n",
                     get_transceiver_suspend_state());
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_GET_MEM:
    cmd_get_memory();
    break;
  case CMD_ID_GET_NET_STATS:
    cmd_get_rmnet_stats();
    break;
  case CMD_ID_GET_GPS_STATS:
    cmd_get_gps_stats();
    break;
  case CMD_ID_GET_HELP:
    cmd_get_help();
    break;

  case CMD_ID_SET_USB_SUSPEND_BLOCK:
    strsz = set_message_response(cmd_id, reply);
    set_suspend_inhibit(true);
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_SET_USB_SUSPEND_ALLOW:
    send_default_response(cmd_id);
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_SET_ADB_ON:
    send_default_response(cmd_id);
    store_adb_setting(true);
    restart_usb_stack();
    break;
  case CMD_ID_SET_ADB_OFF:
    send_default_response(cmd_id);
    store_adb_setting(false);
    restart_usb_stack();
    break;
  case CMD_ID_GET_HISTORY:
    for (uint8_t i = 0; i < cmd_runtime.cmd_position; i++) {
      if (strsz < 160) {
        strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                          "%i ", cmd_runtime.cmd_history[i]);
      }
    }
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_GET_LOG:
    cmd_get_openqti_log();
    break;
  case CMD_ID_GET_KERNEL_LOG:
    cmd_get_kernel_log();
    break;

  case CMD_ID_GET_RECONNECTS:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s: %i\n",
                 bot_commands[cmd_id].cmd_text, get_dirty_reconnects());
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_ACTION_CALL_OWNER:
    send_default_response(cmd_id);
    set_pending_call_flag(true);
    break;
  case CMD_ID_GET_OWNER_NAME:
    strsz +=
        snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%s %s\n",
                 bot_commands[cmd_id].cmd_text, cmd_runtime.user_name);
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_ACTION_POWEROFF:
    pthread_create(&disposable_thread, NULL, &cmd_delayed_shutdown, NULL);
    send_default_response(cmd_id);

    break;
  case CMD_ID_GET_GSM_SIGNAL:
    cmd_render_gsm_signal_data();
    break;
  case CMD_ID_ACTION_REBOOT:
    pthread_create(&disposable_thread, NULL, &cmd_delayed_reboot, NULL);
    send_default_response(cmd_id);

    break;
  case CMD_ID_GET_NET_REPORT:
    cmd_dump_signal_report();
    break;
  case CMD_ID_ACTION_ENABLE_SIGNAL_TRACKING:
    send_default_response(cmd_id);

    enable_signal_tracking(true);
    break;
  case CMD_ID_ACTION_DISABLE_SIGNAL_TRACKING:
    send_default_response(cmd_id);
    enable_signal_tracking(false);
    break;
  case CMD_ID_ACTION_ENABLE_PERSISTENT_LOGGING:
    send_default_response(cmd_id);
    set_persistent_logging(true);
    break;
  case CMD_ID_ACTION_DISABLE_PERSISTENT_LOGGING:
    send_default_response(cmd_id);
    set_persistent_logging(false);
    break;
  case CMD_ID_ACTION_ENABLE_SMS_LOGGING:
    send_default_response(cmd_id);
    set_sms_logging(true);
    break;
  case CMD_ID_ACTION_DISABLE_SMS_LOGGING:
    send_default_response(cmd_id);
    set_sms_logging(false);
    break;
  case CMD_ID_UTIL_LIST_ACTIVE_TASKS: /* List pending tasks */
    dump_pending_tasks();
    break;
  case CMD_ID_GET_FW_TY:
    cmd_thank_you();
    break;
  case CMD_ID_ACTION_ENABLE_CELL_BROADCAST:
    cmd_set_cb_broadcast(true);
    break;
  case CMD_ID_ACTION_DISABLE_CELL_BROADCAST:
    cmd_set_cb_broadcast(false);
    break;
  case CMD_ID_ACTION_CALLWAIT_AUTO_HANGUP:
    send_default_response(cmd_id);
    enable_call_waiting_autohangup(2);
    break;
  case CMD_ID_ACTION_CALLWAIT_AUTO_IGNORE:
    send_default_response(cmd_id);
    enable_call_waiting_autohangup(1);
    break;
  case CMD_ID_ACTION_CALLWAIT_MODE_DEFAULT:
    send_default_response(cmd_id);
    enable_call_waiting_autohangup(0);
    break;
  case CMD_ID_ACTION_ENABLE_CAT:
    send_default_response(cmd_id);
    set_custom_alert_tone(true); // enable in runtime
    break;
  case CMD_ID_ACTION_DISABLE_CAT:
    send_default_response(cmd_id);
    set_custom_alert_tone(false); // enable in runtime
    break;
  case CMD_ID_ACTION_RECORD_CALLS_AND_RECYCLE:
    strsz = set_message_response(cmd_id, reply);
    if (!use_persistent_logging()) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "WARNING: Saving to ram, will be lost on reboot.\n");
    }
    add_message_to_queue(reply, strsz);

    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "NOTICE: Call recording can be forbidden in some jurisdictions. Please "
        "check https://en.wikipedia.org/wiki/Telephone_call_recording_laws for "
        "more details\n");
    add_message_to_queue(reply, strsz);
    set_automatic_call_recording(2);
    break;
  case CMD_ID_ACTION_RECORD_CALLS_ALL:
    strsz = set_message_response(cmd_id, reply);
    if (!use_persistent_logging()) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "WARNING: Saving to ram, will be lost on reboot.\n");
    }
    add_message_to_queue(reply, strsz);
    strsz = snprintf(
        (char *)reply, MAX_MESSAGE_SIZE,
        "NOTICE: Call recording can be forbidden in some jurisdictions. Please "
        "check https://en.wikipedia.org/wiki/Telephone_call_recording_laws for "
        "more details\n");
    add_message_to_queue(reply, strsz);
    set_automatic_call_recording(1);
    break;
  case CMD_ID_ACTION_RECORD_CALLS_NONE:
    send_default_response(cmd_id);
    set_automatic_call_recording(0);
    break;
  case CMD_ID_ACTION_RECORD_CALL_CURRENT:
    if (record_current_call() == 0) {
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE, "%s\n",
                       bot_commands[cmd_id].cmd_text);
      if (!use_persistent_logging()) {
        strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                          "WARNING: Saving to ram, will be lost on reboot.\n");
      }
    } else
      strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                       "There is no call in progress!\n");
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_ACTION_RECORD_CALL_NEXT:
    send_default_response(cmd_id);
    record_next_call(true);
    break;
  case CMD_ID_ACTION_RECORD_CALL_CANCEL_TASK:
    send_default_response(cmd_id);
    record_next_call(false);
    break;
  case CMD_ID_DEBUG_GET_MM_SAMPLE_GSM7_CB_MSG:
    cmd_debug_gsm7_cb_message(command);
    break;
  case CMD_ID_DEBUG_GET_MM_SAMPLE_UCS2_CB_MSG:
    cmd_debug_ucs2_cb_message(command);
    break;
  case CMD_ID_ACTION_ENABLE_INTERNAL_NETWORKING:
    send_default_response(cmd_id);
    set_internal_connectivity(true);
    init_internal_networking();
    break;
  case CMD_ID_ACTION_DISABLE_INTERNAL_NETWORKING: // disable smdcntl0
    send_default_response(cmd_id);
    set_internal_connectivity(false);
    break;
  case CMD_ID_ACTION_DISABLE_DO_NOT_DISTURB:
    send_default_response(cmd_id);
    set_do_not_disturb(false);
    remove_all_tasks_by_type(TASK_TYPE_DND_CLEAR);
    break;
  case CMD_ID_ACTION_DEBUG_DISABLE_SVC:
    send_default_response(cmd_id);
    disable_service_debugging();
    break;
  case CMD_ID_GET_KNOWN_QMI_SERVICES:
    strsz = 0;
    snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
             "Known QMI Services\n");
    add_message_to_queue(reply, strsz);
    strsz = 0;
    for (uint16_t i = 0; i < (sizeof(qmi_services) / sizeof(qmi_services[0]));
         i++) {
      if (strlen(qmi_services[i].name) + (3 * sizeof(uint8_t)) +
              strlen(qmi_services[i].name) + strsz >
          MAX_MESSAGE_SIZE) {
        add_message_to_queue(reply, strsz);
        strsz = 0;
      }
      strsz +=
          snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz, "%u: %s\n",
                   qmi_services[i].service, qmi_services[i].name);
    }
    add_message_to_queue(reply, strsz);
    break;
  case CMD_ID_ACTION_DEBUG_ENABLE_NETWORK_DATA_DUMP:
    send_default_response(cmd_id);
    enable_dump_network_tables(true);
    break;
  case CMD_ID_ACTION_ENABLE_ST_NETWORK_DOWNGRADE_NOTIF:
    send_default_response(cmd_id);
    set_signal_tracking_downgrade_notification(true);
    break;
  case CMD_ID_ACTION_DISABLE_ST_NETWORK_DOWNGRADE_NOTIF:
    send_default_response(cmd_id);
    set_signal_tracking_downgrade_notification(false);
    break;
  case CMD_ID_ACTION_DEBUG_DISABLE_NETWORK_DATA_DUMP:
    send_default_response(cmd_id);
    enable_dump_network_tables(false);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_CLEAR_AUTH_DATA:
    cmd_clear_internal_networking_auth();
    break;
  case CMD_ID_ACTION_MESSAGE_RECOVERY_ENABLE:
    send_default_response(cmd_id);
    set_list_all_bypass(true);
    break;
  case CMD_ID_ACTION_MESSAGE_RECOVERY_DISABLE:
    send_default_response(cmd_id);
    set_list_all_bypass(false);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_START:
    send_default_response(cmd_id);
    wds_chat_ifup_down(true);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_STOP:
    send_default_response(cmd_id);
    wds_chat_ifup_down(false);
    break;

    /* Partials */
  case CMD_ID_SET_MODEM_NAME:
    cmd_set_custom_modem_name(command);
    break;
  case CMD_ID_SET_OWNER_NAME:
    cmd_set_custom_user_name(command);
    break;
  case CMD_ID_ACTION_CALL_OWNER_COUNTDOWN:
    pthread_create(&disposable_thread, NULL, &cmd_schedule_call, command);
    sleep(2); // our string gets wiped out before we have a chance
    break;
  case CMD_ID_ACTION_SET_REMINDER:
    cmd_schedule_reminder(command);
    break;
  case CMD_ID_ACTION_WAKE_OWNER:
    cmd_schedule_wakeup(command);
    break;
  case CMD_ID_ACTION_DELETE_TASK: /* Delete task %i */
    cmd_delete_task(command);
    break;
  case CMD_ID_ACTION_ENABLE_DND: /* Leave me alone [not implemented yet] */
    cmd_suspend_call_notifications(command);
    break;
  case CMD_ID_DEBUG_ENABLE_SERVICE_LOGGING:
    cmd_enable_service_debugging_for_service_id(command);
    break;
  case CMD_ID_ACTION_SET_ST_MODE:
    cmd_set_new_signal_tracking_mode(command);
    break;
  case CMD_ID_ACTION_SET_ST_NOTIFICATION_LEVEL:
    cmd_configure_signal_tracking_cell_notification(command);
    break;
  case CMD_ID_DEFINE:
    cmd_search_dictionary_entry(command);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_SET_APN:
    cmd_configure_new_apn(command);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_SET_USER:
    cmd_configure_apn_username(command);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_SET_PASS:
    cmd_configure_apn_password(command);
    break;
  case CMD_ID_ACTION_INTERNAL_NETWORK_SET_AUTH_METHOD:
    cmd_configure_internal_network_auth_method(command);
    break;
  case CMD_UNKNOWN:
  default:
    strsz = snprintf((char *)reply, MAX_MESSAGE_SIZE,
                      "Invalid command id %i\n", cmd_id);
    logger(MSG_INFO, "%s: Unknown command %i\n", __func__, cmd_id);
    break;
  }

  add_to_history(cmd_id);
  return ret;
}
