// SPDX-License-Identifier: MIT
#ifndef _CELL_H_
#define _CELL_H_

#include "../inc/openqti.h"
#include <stdbool.h>
#include <stdio.h>

/*
AT+QENG="servingcell"
+QENG:
"servingcell","NOCONN","LTE","FDD",214,01,7676A09,188,6300,20,3,3,603,-104,-9,-78,11,23

OK
AT+QENG="neighbourcell"
+QENG: "neighbourcell intra","LTE",6300,188,-12,-105,-75,-20,23,4,12,10,62
+QENG: "neighbourcell inter","LTE",1501,-,-,-,-,-,-,0,10,7
+QENG: "neighbourcell inter","LTE",3250,-,-,-,-,-,-,0,16,5
+QENG: "neighbourcell","WCDMA",3062,3,16,2,-,-,-,-
+QENG: "neighbourcell","WCDMA",10713,3,16,2,-,-,-,-
*/

struct cell_data {
  uint8_t mcc;
  uint8_t mnc;
  uint8_t network_type; // LTE / WCDMA / GSM / ??
};
#endif