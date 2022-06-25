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
} __attribute__((packed));
/* We need plenty of things, I'll slowly try to organize them here */

/* Services */
enum {
    DIAG_CON_APSS = 0x0001,
    DIAG_CON_MPSS = 0x0002,
    DIAG_CON_LPASS = 0x0004,
    DIAG_CON_WCNSS = 0x0008,
    DIAG_CON_SENSORS = 0x0010,
    DIAG_CON_NONE = 0x0000,
    DIAG_CON_ALL = (DIAG_CON_APSS | DIAG_CON_MPSS | DIAG_CON_LPASS \
                  | DIAG_CON_WCNSS | DIAG_CON_SENSORS)

};

/* Commands */

#endif