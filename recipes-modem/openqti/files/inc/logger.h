// SPDX-License-Identifier: MIT
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "../inc/openqti.h"
#include <stdbool.h>
#include <stdio.h>

#define VOLATILE_LOGPATH "/var/log/openqti.log"
#define PERSISTENT_LOGPATH "/persist/openqti.log"


#define VOLATILE_THERMAL_LOGFILE "/var/log/thermal.log"
#define PERSISTENT_THERMAL_LOGFILE "/persist/thermal.log"

void reset_logtime();
double get_elapsed_time();
void logger(uint8_t level, char *format, ...);
void log_thermal_status(uint8_t level, char *format, ...);
void dump_packet(char *direction, uint8_t *buf, int pktsize);
void dump_pkt_raw(uint8_t *buf, int pktsize);
void pretty_print_qmi_pkt(char *direction, uint8_t *buf, int pktsize);
uint8_t get_log_level();
void set_log_level(uint8_t level);
void set_log_method(bool ttyout);
int mask_phone_number(uint8_t *orig, char *dest, uint8_t len);
#endif

