/* SPDX-License-Identifier: MIT */

#ifndef __SPACE_MONITOR_H__
#define __SPACE_MONITOR_H__

#include <stdbool.h>
#include <stdint.h>

#define RAM_STORAGE_PATH_BASE "/tmp"
#define PERSISTENT_STORAGE_PATH_BASE "/persist"


int get_available_space_tmpfs_mb();
int get_available_space_persist_mb();
int cleanup_storage(int storage_id, bool aggressive, char *exclude_file);
#endif
