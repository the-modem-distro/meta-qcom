/* SPDX-License-Identifier: MIT */

#ifndef _DIAGMONITOR_H_
#define _DIAGMONITOR_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define DIAG_INTERFACE "/dev/diag"
#define MAX_BUF_SIZE 32768 // diagchar.h says 16384

struct diag_logging_mode_param_t {
        uint32_t req_mode;
        uint32_t peripheral_mask;
        uint8_t mode_param;
} __packed;
/* We need plenty of things, I'll slowly try to organize them here */


#endif