// SPDX-License-Identifier: MIT
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdbool.h>
#include <stdio.h>
#include "../inc/openqti.h"

void logger(uint8_t level, char *format, ...);
void dump_packet( char *direction, uint8_t *buf, int pktsize);
void set_log_level(uint8_t level);
void set_log_method(bool ttyout);
#endif