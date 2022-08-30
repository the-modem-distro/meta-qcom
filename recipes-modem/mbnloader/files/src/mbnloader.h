/* SPDX-License-Identifier: MIT */

#ifndef _MBNLOADER_H
#define _MBNLOADER_H

#include <stdbool.h>
#include <stdint.h>

#define SMD_DEVICE_PATH "/dev/ttyUSB2"
#define UPLOAD_CMD "AT+QFUPL=" //"RAM:test.mbn",40160
#define DELETE_CMD "AT+QFDEL="
#define LOAD_AS_MBN_CMD "AT+QMBNCFG=\"Add\",\"volte_profile.mbn\"\n\r"
#define SET_AUTOSEL_CMD "AT+QMBNCFG=\"Autosel\",1\n\r"
#define LIST_FILES_CMD "AT+QMBNCFG=\"List\""

#endif
