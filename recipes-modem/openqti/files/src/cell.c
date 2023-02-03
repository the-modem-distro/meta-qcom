// SPDX-License-Identifier: MIT

#include "../inc/cell.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include "../inc/sms.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_RESPONSE_SZ 4096
/*
 * cell data
 *  We're going to exploit Quectel's engineering commands
 *  to track signal status, network mode and servicing and
 *  neighbour cells
 *
 *  If we get a sudden change in neighbour cells, service drop
 *  etc. we should be able to track it down here
 */

struct network_state net_status;

struct {
  uint8_t history_sz;
  struct cell_report current_report;
  struct cell_report history[128];
} report_data;

/*

Return last reported network type
0x00 -> No service
0x01 -> CDMA
0x02 -> CDMA EVDO
0x03 -> AMPS
0x04 -> GSM
0x05 -> UMTS
0x06 ??
0x07 ??
0x08 LTE

*/


struct network_state get_network_status() {
  return net_status;
}

struct cell_report get_current_cell_report() {
  return report_data.current_report;
}


