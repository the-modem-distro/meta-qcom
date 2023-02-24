#include "space_mon.h"
#include "config.h"
#include "logger.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/vfs.h>

int get_available_space_tmpfs_mb() {
  struct statfs sStats;
  if (statfs("/tmp", &sStats) == -1) {
    logger(MSG_INFO, "statfs() failed\n");
    return -EINVAL;
  } else {
    return (sStats.f_bsize * sStats.f_bavail) / 1024 / 1024;
  }
}

int get_available_space_persist_mb() {
  struct statfs sStats;
  uint32_t size;
  if (statfs("/persist", &sStats) == -1) {
    logger(MSG_INFO, "statfs() failed\n");
    return -EINVAL;
  } else {
    size = (sStats.f_bsize * sStats.f_bavail) / 1024 / 1024;
    return size;
  }
}

int cleanup_storage(int storage_id, bool aggressive, char *exclude_file) {
  char *path;
  char tmpfile[512];
  DIR *dp;
  struct dirent *ep;
  path = calloc(512, sizeof(char));
  logger(MSG_ERROR,
         "%s called, Storage ID %i, is_aggressive? %i, priority file: %s\n",
         __func__, storage_id, aggressive, exclude_file);
  switch (storage_id) {
  case 0: // RAM
    memcpy(path, RAM_STORAGE_PATH_BASE, strlen(RAM_STORAGE_PATH_BASE));
    break;
  case 1: // Persistent storage
    memcpy(path, PERSISTENT_STORAGE_PATH_BASE,
            strlen(PERSISTENT_STORAGE_PATH_BASE));
    break;
  default:
    logger(MSG_ERROR, "%s: Invalid storage selected for cleanup\n", __func__);
    free(path);
    return -EINVAL;
    break;
  }

  dp = opendir(path);
  if (dp != NULL) {
    logger(MSG_ERROR, "Files available for removal\n");
    while ((ep = readdir(dp))) {
      memset(tmpfile, 0, 512);
      snprintf(tmpfile, 512, "%s/%s", path, ep->d_name);
      if (strcmp(ep->d_name, "..") != 0 && 
          strcmp(ep->d_name, ".") != 0 && 
          strcmp(ep->d_name, "openqti.conf") != 0 &&
          strcmp(ep->d_name, "openqti.lock") != 0 &&
          strstr(ep->d_name, ".bin") == NULL &&
          strstr(ep->d_name, ".raw") == NULL ) {
        logger(MSG_DEBUG, "Found file: %s\n", tmpfile);
        if (aggressive) {
          if (strstr(ep->d_name, exclude_file) == NULL) { 
                    // The file name comes from the incall thread with the
                          // entire path
            logger(MSG_ERROR, "Deleting %s\n", ep->d_name);
            remove(tmpfile);
          }
        } else if (strstr(ep->d_name, "log") != NULL || strstr(ep->d_name, "csv") != NULL) {
          logger(MSG_ERROR, " DEL %s\n", ep->d_name);
          remove(tmpfile);
        }
      }
    }
  } else {
    logger(MSG_ERROR, "%s: Error opening directory %s\n", __func__, path);
  }
  closedir(dp);
  free(path);

  return 0;
}