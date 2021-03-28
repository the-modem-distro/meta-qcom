// SPDX-License-Identifier: MIT

#include "../inc/ipc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

int open_ipc_socket(struct qmi_device *qmisock, uint32_t node, uint32_t port,
                    uint32_t service, uint32_t instance,
                    unsigned char address_type) {
  qmisock->fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  qmisock->service = service;
  qmisock->transaction_id = 1;
  qmisock->socket.family = IPC_ROUTER;
  qmisock->socket.address.addrtype = address_type;
  qmisock->socket.address.addr.port_addr.node_id = node;
  qmisock->socket.address.addr.port_addr.port_id = port;
  qmisock->socket.address.addr.port_name.service = service;
  qmisock->socket.address.addr.port_name.instance = instance;

  return qmisock->fd;
}

bool is_server_active(uint32_t service, uint32_t instance) {
  int sock, i;
  bool ret = false;
  struct server_lookup_args *lookup;
  lookup =
      (struct server_lookup_args *)calloc(2, sizeof(struct server_lookup_args));

  sock = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  lookup->port_name.service = service;
  lookup->port_name.instance = instance;
  if (instance == 0) {
    lookup->lookup_mask = 0;
  } else {
    lookup->lookup_mask = 0xFFFFFFFF;
  }
  lookup->num_entries_in_array =
      1; // In reality this is the number of entries to fill
  lookup->num_entries_found = 0;
  if (ioctl(sock, IPC_ROUTER_IOCTL_LOOKUP_SERVER, lookup) >= 0) {
    for (i = 0; i < lookup->num_entries_in_array; i++) {
      if (lookup->srv_info[i].node_id != 41 && i == 0) {
        ret = true;
      }
    }
  }
  close(sock);

  free(lookup);
  lookup = NULL;
  return ret;
}

int find_services() {
  int i, j, k, fd;
  bool name = false;
  uint32_t instance = 1;
  int ret = 0;
  struct server_lookup_args *lookup;
  lookup =
      (struct server_lookup_args *)calloc(2, sizeof(struct server_lookup_args));
  fprintf(stdout, "Service Instance Node    Port \t Name \n");
  fprintf(stdout, "--------------------------------------------\n");
  for (k = 1; k <= 4097; k++) {
    name = false;
    fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
    if (fd < 0) {
      fprintf(stdout, "Error opening socket\n");
      return -EINVAL;
    }
    lookup->port_name.service = k;
    lookup->port_name.instance = instance;
    lookup->num_entries_in_array = 1;
    lookup->num_entries_found = 0;
    if (instance == 0) {
      lookup->lookup_mask = 0;
    } else {
      lookup->lookup_mask = 0xFFFFFFFF;
    }
    if (ioctl(fd, IPC_ROUTER_IOCTL_LOOKUP_SERVER, lookup) < 0) {

      fprintf(stdout, "ioctl failed\n");
      close(fd);
      free(lookup);
      return -EINVAL;
    }

    for (i = 0; i < lookup->num_entries_in_array; i++) {
      if (lookup->srv_info[i].port_id != 0x0e &&
          lookup->srv_info[i].port_id != 0x0b) {
        fprintf(stdout, "%i \t %i \t 0x%.2x \t 0x%.2x \t", k, instance,
               lookup->srv_info[i].node_id, lookup->srv_info[i].port_id);
        for (j = 0; j < (sizeof(common_names) / sizeof(common_names[0])); j++) {
          if (common_names[j].service == k) {
            fprintf(stdout, " %s\n", common_names[j].name);
            name = true;
          }
        }
        if (!name) {
          fprintf(stdout, " Unknown service \n");
        }
        if (i > 0) {
          fprintf(stdout, 
              "Hey we have more than one port for the same service or what?\n");
        }
      }
    }
    close(fd);
  }

  free(lookup);
  lookup = NULL;
  return 0;
}

struct msm_ipc_server_info get_node_port(uint32_t service, uint32_t instance) {
  struct server_lookup_args *lookup;

  int sock, i;
  bool ret = false;
  struct msm_ipc_server_info port_combo;

  port_combo.node_id = 0;
  port_combo.port_id = 0;
  sock = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  lookup =
      (struct server_lookup_args *)calloc(2, sizeof(struct server_lookup_args));
  lookup->port_name.service = service;
  lookup->port_name.instance = instance;
  lookup->num_entries_in_array = 1;
  lookup->num_entries_found = 0;
  if (instance == 0) {
    lookup->lookup_mask = 0;
  } else {
    lookup->lookup_mask = 0xFFFFFFFF;
  }

  if (ioctl(sock, IPC_ROUTER_IOCTL_LOOKUP_SERVER, lookup) >= 0) {
    if (lookup->num_entries_in_array > 0) {
      if (lookup->srv_info[0].node_id != 41) {
        port_combo.node_id = lookup->srv_info[0].node_id;
        port_combo.port_id = lookup->srv_info[0].port_id;
        port_combo.service = lookup->srv_info[0].service;
        port_combo.instance = lookup->srv_info[0].instance;

        ret = true;
      }
    }
  }

  free(lookup);
  close(sock);
  lookup = NULL;
  return port_combo;
}