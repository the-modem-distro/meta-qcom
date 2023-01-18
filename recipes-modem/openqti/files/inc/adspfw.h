// SPDX-License-Identifier: MIT
#ifndef _ADSPFW_H_
#define _ADSPFW_H_

#include "../inc/openqti.h"
#include <stdbool.h>
#include <stdio.h>

#define ADSPFW_GEN_FILE "/lib/firmware/modem.b12"

static const struct {
  unsigned int id;
  char variant;
  const char *fwstring;
  const char *md5sum;
} known_adsp_fw[] = {
    {0, '0', "Unknown ADSP Firmware", "Unknown__"},
    {1, '3', "EG25GGBR07A08M2G_01.003.01.003", "8dcfc9dfc39d4bb41f86014ca6b54040"},
    {2, '2', "EG25GGBR07A08M2G_01.002.07", "835030d95d471d4557caa13f14eb5626"},
    {3, '1', "EG25GGBR07A08M2G_01.001.10", "64bf3d938fb21a0104d01fcacb37bc8e"},
    {4, '1', "EG25GGBR07A07M2G_01.001.02", "6a92799327a56905f6496613671f56df"},
    {5, '1', "EG25GGBR07A07M2G_OCPU_01.001.01.001", "00f6763a96bc702eaf710cc928b6fa99"},
    {6, '4', "EG25GGBR07A08M2G_30.004.30.004", "74fcbcdd4abed713ce8cd3f8818e7afd"},   
    {7, '6', "EG25GGBR07A08M2G_30.006.30.006", "2206b35df4cc1c51a06b1bb25729a0cd"},   
    {7, '7', "EG25GGBR07A08M2G_30.007.30.007", "3ad071d36b20fcde95efcf6eaa1005d6"},   
};

#endif
