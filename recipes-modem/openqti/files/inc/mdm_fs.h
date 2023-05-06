// SPDX-License-Identifier: MIT
#ifndef __PDC_H__
#define __PDC_H__

#include "openqti.h"
#include "qmi.h"
#include "call.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Modemfs service
 *
 *
 *
 */

enum {
	 MODEM_FS_READ_SUPPORTED_MSGS = 0x001E, 
	 MODEM_FS_READ_SUPPORTED_FIELDS = 0x001F, 
	 MODEM_FS_WRITE = 0x0020, 
	 MODEM_FS_READ = 0x0021,
};

static const struct {
    uint16_t id;
    const char *cmd;
} mdmfs_svc_commands[] = {
    {MODEM_FS_READ_SUPPORTED_MSGS,"MODEM_FS_READ_SUPPORTED_MSGS"}, 
    {MODEM_FS_READ_SUPPORTED_FIELDS,"MODEM_FS_READ_SUPPORTED_FIELDS"}, 
    {MODEM_FS_WRITE,"MODEM_FS_WRITE"}, 
    {MODEM_FS_READ,"MODEM_FS_READ"}, 
};

/* List settings stuff */
struct modemfs_path {
  uint8_t id; // 0x01
  uint16_t len; // variable
  uint8_t path[0]; // Path to file
} __attribute__((packed));

struct modemfs_readsz { //PDC_CONFIG_HW
  uint8_t id; // 0x10
  uint16_t len;
  uint32_t chunksz;
} __attribute__((packed));

// 0x02 

const char *get_mdmfs_command(uint16_t msgid);
void *register_to_mdmfs_service();
int handle_incoming_mdmfs_message(uint8_t *buf, size_t buf_len);
#endif
