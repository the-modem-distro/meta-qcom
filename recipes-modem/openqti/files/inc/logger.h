// SPDX-License-Identifier: MIT
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdbool.h>
#include <stdio.h>
#include "../inc/openqti.h"

void logger(bool debug_to_stdout, uint8_t level, char *format, ...);
void dump_packet(bool debug_to_stdout, char *direction, char *buf);

#endif