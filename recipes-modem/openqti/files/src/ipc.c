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


