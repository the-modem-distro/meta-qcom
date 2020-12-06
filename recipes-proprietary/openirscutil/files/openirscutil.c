#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/ioctl.h>

/*
 * Dead simple IPC Router Secutiry Feeder
 * Use with no arguments to allow group id 54 access to the IPC
 * Use openirscutil [GROUPID] to specify another group
 */
struct rule {
   int rl_no;
   uint32_t service;
   uint32_t instance;
   unsigned dummy;
   gid_t group_id[0];
};

#define IOCTL_RULES	_IOR(0xC3, 5,  struct rule)

int main(int argc, char **argv)
{
  int exit_code = 0;
  int ipc_categories = 511;
  int group_id = 54;
  int fd;
  int i;
  struct rule *cur_rule;
  printf("Dead simple IPC Router Security feeder\n");
  if (argc > 1) {
    if (atoi(argv[1]) > 0 && atoi(argv[1]) < 65536) {
      group_id = atoi(argv[1]);
      printf("Setting GID %i as allowed \n", group_id);
    }
  } else {
    printf("No arguments passed, setting GID %i as allowed \n", group_id);
  }
  fd = socket(27, SOCK_DGRAM, 0);
  if (!fd) {
    printf(" Error opening socket \n");
    return -1;
  } 
  for (i = 0; i < ipc_categories; i++) {
    cur_rule = calloc(1, (sizeof(*cur_rule) + sizeof(uint32_t)));
    cur_rule->rl_no = 54;
    cur_rule->service = i;
    cur_rule->instance = 4294967295; // all instances
    cur_rule->group_id[0] = group_id;
   if (ioctl(fd, IOCTL_RULES, cur_rule) < 0) {
      printf("Error serring rule %i \n", i);
      exit_code = 2;
      break;
    }
  }

  close(fd);
  if (exit_code != 0) {
    printf("Error uploading rules (%i) \n", exit_code);
  } else {
    printf("Upload finished. \n");
  }

  return exit_code;
}
