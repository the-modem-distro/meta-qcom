// SPDX-License-Identifier: MIT

#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/call.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/proxy.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"
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
