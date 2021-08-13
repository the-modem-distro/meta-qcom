/* SPDX-License-Identifier: MIT */

#ifndef _TRACKING_H_
#define _TRACKING_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// Client release command request
#define CLIENT_REGISTER_REQ 0x22
#define CLIENT_RELEASE_REQ 0x23
#define CLIENT_REG_TIMEOUT 2400000

#define HOST_USES_MODEMMANAGER 0x1a
#define HOST_USES_OFONO 0x02

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

#endif