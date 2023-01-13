/* SPDX-License-Identifier: MIT */

#ifndef _TRACKING_H_
#define _TRACKING_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/* QMI Control service Message IDs */
enum {
  CONTROL_SET_INSTANCE = 0x0020,
  CONTROL_GET_VERSION = 0x0021,
  CONTROL_CLIENT_REGISTER_REQ = 0x0022,
  CONTROL_CLIENT_RELEASE_REQ = 0x0023,
  CONTROL_REVOKE_CLIENT_REQ = 0x0024,
  CONTROL_NOTIFY_INVALID_CLIENT = 0x0025,
  CONTROL_SET_DATA_FORMAT = 0x0026,
  CONTROL_SYNC_SERVICE = 0x0027,
  CONTROL_POWERSAVE_INDICATION = 0x0028,
  CONTROL_POWERSAVE_MODE_SETTINGS = 0x0029,
  CONTROL_SET_POWERSAVE_MODE = 0x002a,
  CONTROL_GET_POWERSAVE_MODE = 0x002b,
};

static const struct {
  uint16_t id;
  const char *cmd;
} control_service_commands[] = {
    { CONTROL_SET_INSTANCE, "Set instance"},
    { CONTROL_GET_VERSION, "Get version"},
    { CONTROL_CLIENT_REGISTER_REQ, "Client register request"},
    { CONTROL_CLIENT_RELEASE_REQ, "Client release request"},
    { CONTROL_REVOKE_CLIENT_REQ, "Client revoke request"},
    { CONTROL_NOTIFY_INVALID_CLIENT, "Notify invalid client"},
    { CONTROL_SET_DATA_FORMAT, "Set data format"},
    { CONTROL_SYNC_SERVICE, "Sync service state"},
    { CONTROL_POWERSAVE_INDICATION, "Notify: powersave state"},
    { CONTROL_POWERSAVE_MODE_SETTINGS, "Powersave mode settings"},
    { CONTROL_SET_POWERSAVE_MODE, "Set powersave mode"},
    { CONTROL_GET_POWERSAVE_MODE, "Get powersave mode"},
};

#define CLIENT_REG_TIMEOUT 2400000

#define HOST_USES_MODEMMANAGER 0x1a
#define HOST_USES_OFONO 0x02
#define MAX_ACTIVE_CLIENTS 22 // ModemManager uses 9-10

void reset_client_handler();
int get_num_instances_for_service(int service);
bool is_register_release_request(uint8_t *pkt, int sz);
int add_client(uint8_t service, uint8_t instance);
int remove_client(uint8_t service, uint8_t instance);
int set_current_host_app(uint8_t app);
void update_register_time();
uint8_t get_current_host_app();
int track_client_count(uint8_t *pkt, int from, int sz, int fd, int rmnet_fd);
void force_close_qmi(int fd);
void send_rmnet_ioctls(int fd);
void reset_client_handler();
void reset_dirty_reconnects();
uint8_t get_dirty_reconnects();
const char *get_ctl_command(uint16_t msgid);
#endif