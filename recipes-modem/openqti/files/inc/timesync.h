/* SPDX-License-Identifier: MIT */

#ifndef _TIMESYNC_H_
#define _TIMESYNC_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>

#define SET_CTZU "AT+CTZU=3\r"
#define GET_QLTS "AT+QLTS\r"
#define GET_CCLK "AT+CCLK?\r"


int get_timezone();
bool is_timezone_offset_negative();
void *time_sync();
#endif