#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "../inc/ipc.h"

int open_ipc_socket(struct qmi_device * qmisock, 
                    uint32_t node, 
                    uint32_t port,
                    uint32_t service, 
                    uint32_t instance, 
                    unsigned char address_type
                    ) {
  qmisock -> fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  qmisock -> service = service;
  qmisock -> transaction_id = 1;
  qmisock -> socket.family = IPC_ROUTER;
  qmisock -> socket.address.addrtype = address_type;
  qmisock -> socket.address.addr.port_addr.node_id = node;
  qmisock -> socket.address.addr.port_addr.port_id = port;
  qmisock -> socket.address.addr.port_name.service = service;
  qmisock -> socket.address.addr.port_name.instance = instance;

  return qmisock -> fd;
}

bool is_server_active(uint32_t node, uint32_t port) {
  bool ret = false;
  int fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  if (fd < 0) {
    return false;
  }
  struct sockaddr_msm_ipc * socket;
  socket = (struct sockaddr_msm_ipc *) calloc(1, sizeof(struct sockaddr_msm_ipc));
  socket->address.addr.port_addr.node_id = node;
  socket->address.addr.port_addr.port_id = port;
  if (ioctl(fd, IPC_ROUTER_IOCTL_LOOKUP_SERVER, socket) == 0) {
    ret = true;
  }
  
  close(fd);
  return ret;
}

int find_and_connect_to(struct qmi_device *qmidev,uint32_t address_type, uint32_t service, uint32_t instance) {
  struct server_lookup_args *lookup;
  lookup = (struct server_lookup_args *)calloc(1, sizeof(struct server_lookup_args));
  int i,j;
  //struct qmi_device * qmidev;
  qmidev = (struct qmi_device *)calloc(1, sizeof(struct qmi_device));

  qmidev -> fd = socket(IPC_ROUTER, SOCK_DGRAM, 0);
  lookup->port_name.service = service;
  lookup->port_name.instance = instance;
  lookup->lookup_mask = 0;
  lookup->num_entries_in_array = 1;
  lookup->num_entries_found = 0;
  if (ioctl(qmidev->fd, IPC_ROUTER_IOCTL_LOOKUP_SERVER, lookup) < 0) {
    printf("ioctl failed\n");
    return -EINVAL;
  }
  qmidev -> service = service;
  qmidev -> transaction_id = 1;
  qmidev -> socket.family = IPC_ROUTER;
  qmidev -> socket.address.addrtype = address_type; //except for DPM, ADDRTYPE seems to be always 2
  qmidev -> socket.address.addr.port_name.service = service;
  qmidev -> socket.address.addr.port_name.instance = instance;
  qmidev -> socket.address.addr.port_addr.node_id =  0;
  qmidev -> socket.address.addr.port_addr.port_id =  0;
    
  for (i = 0; i < lookup->num_entries_in_array; i++ ) {
    if (lookup->srv_info[i].node_id != 41) {
    printf("looking for service %i: ", service);
    for (j = 0; j < (sizeof(common_names) / sizeof(common_names[0]));j++ ) {
      if (common_names[j].service == service) {
        printf("-> %s", common_names[j].name);
      }
    }
    printf(": Node %i, Port %.2x\n", lookup->srv_info[i].node_id, lookup->srv_info[i].port_id);
    qmidev -> socket.address.addr.port_addr.node_id =  lookup->srv_info[i].node_id;
    qmidev -> socket.address.addr.port_addr.port_id =  lookup->srv_info[i].port_id;
    if (i > 0) {
      printf("Hey we have more than one port for the same service or what?\n");
    }
    } else {
      return -EINVAL;
    }

  }
  return 0;
}