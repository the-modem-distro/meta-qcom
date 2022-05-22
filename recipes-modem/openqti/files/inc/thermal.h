/* SPDX-License-Identifier: MIT */

#ifndef _THERMAL_H_
#define _THERMAL_H_

#include <stdbool.h>
#include <stdint.h>

#define NO_OF_SENSORS 6
#define THRM_ZONE_PATH "/sys/devices/virtual/thermal/thermal_zone"
#define THRM_ZONE_TRAIL "/temp"

void *thermal_monitoring_thread();

#endif