// SPDX-License-Identifier: MIT

#include "../inc/cell.h"
#include "../inc/logger.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

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
uint8_t get_network_type() {
    return net_status.network_type;
}
/* Returns last reported signal strength, in dB */
uint8_t get_signal_strength() {
    return net_status.signal_level;
}

void update_network_data(uint8_t network_type, uint8_t signal_level) {
    net_status.network_type = network_type;
    net_status.signal_level = signal_level;
}