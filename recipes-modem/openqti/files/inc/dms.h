// SPDX-License-Identifier: MIT
#ifndef __DMS_H__
#define __DMS_H__

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Device Management Service
 *
 *
 *
 */


/* DMS Testing */
enum {
  DMS_RESET = 0x0000,
  DMS_SET_EVENT_REPORT = 0x0001,
  DMS_REGISTER = 0x0003,
  DMS_GET_MODEL = 0x0022,
  DMS_GET_REVISION = 0x0023, /* 10 boot, 11 firmware, 01 revision*/
  DMS_GET_SERIAL_NUM = 0x0025,
  DMS_GET_TIME = 0x002f,
  DMS_SET_TIME = 0x004b,
  DMS_GET_HARDWARE_REVISION = 0x002c,

};


static const struct {
    uint16_t id;
    const char *cmd;
  } dms_svc_commands[] = {
    { DMS_RESET, "Device Management Service Reset"},
    { DMS_SET_EVENT_REPORT, "DMS: Set Event Report"},
    { DMS_REGISTER, "DMS: Register"},
    { DMS_GET_MODEL, "DMS: Get Model"},
    { DMS_GET_REVISION, "DMS: Get Modem Revision"},
    { DMS_GET_SERIAL_NUM, "DMS: Get Modem Serial Number"},
    { DMS_GET_TIME, "DMS: Get current time from the baseband"},
    { DMS_SET_TIME, "DMS: Set current time"},
    { DMS_GET_HARDWARE_REVISION, "DMS: Get HW Rev"},
};
/* External functions for the chat */
const char *dms_get_modem_revision();
const char *dms_get_modem_modem_serial_num();
const char *dms_get_modem_modem_manufacturer();
const char *dms_get_modem_modem_hw_rev();
const char *dms_get_modem_modem_model();





void reset_dms_runtime();
void *dms_get_modem_info();
const char *get_dms_command(uint16_t msgid);

int handle_incoming_dms_message(uint8_t *buf, size_t buf_len);
#endif
