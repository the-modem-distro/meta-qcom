// SPDX-License-Identifier: MIT

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/mdm_fs.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/qmi.h"

#define DEBUG_MDMFS 1

const char *get_mdmfs_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(mdmfs_svc_commands) / sizeof(mdmfs_svc_commands[0])); i++) {
    if (mdmfs_svc_commands[i].id == msgid) {
      return mdmfs_svc_commands[i].cmd;
    }
  }
  return "MDM_FS: Unknown command\n";
}

int mdmfs_request_read_file(uint8_t *path) {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   sizeof(struct modemfs_path) + strlen((char*)path);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_MDMFS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, MODEM_FS_READ) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  struct modemfs_path *filepath = (struct modemfs_path *)(pkt + curr_offset);
  filepath->id = 0x01;
  filepath->len = strlen((char*)path);
  memcpy(filepath->path, path, filepath->len);


  add_pending_message(QMI_SERVICE_MDMFS, (uint8_t *)pkt, pkt_len);

  free(pkt);
    return 0;
}

int mdmfs_demo_read() {
    char demopath[]= "/nv/item_files/modem/mmode/sms_domain_pref";
    mdmfs_request_read_file((uint8_t *)demopath);
  return 0;
}

/*
int mdmfs_request_config_list() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   sizeof(struct mdmfs_indication_key) + sizeof(struct mdmfs_setting_type);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_PDC, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       PERSISTENT_DEVICE_CONFIG_LIST_CONFIGS) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  struct mdmfs_indication_key *key = (struct mdmfs_indication_key *)(pkt + curr_offset);
  key->id = 0x10;
  key->len = sizeof(uint32_t);
  key->key = 0xdeadbeef;

  curr_offset += sizeof(struct mdmfs_indication_key);
  struct mdmfs_setting_type *config_type = (struct mdmfs_setting_type *)(pkt + curr_offset);
  config_type->id = 0x11;
  config_type->len = sizeof(uint32_t);
  config_type->data = PDC_CONFIG_SW;

  add_pending_message(QMI_SERVICE_PDC, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}*/

/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_mdmfs_message(uint8_t *buf, size_t buf_len) {
   logger(MSG_INFO, "%s: Start: Message from the MDMFS Service: %.4x | %s\n",
           __func__, get_qmi_message_id(buf, buf_len), get_mdmfs_command(get_qmi_message_id(buf, buf_len)));

#ifdef DEBUG_MDMFS
  pretty_print_qmi_pkt("MDMFS: Baseband --> Host", buf, buf_len);
#endif

  switch (get_qmi_message_id(buf, buf_len)) {
    case MODEM_FS_READ:
    case MODEM_FS_WRITE:
    default:
        logger(MSG_INFO, "%s: Unhandled message for the MDMFS Service: %.4x | %s\n",
            __func__, get_qmi_message_id(buf, buf_len), get_mdmfs_command(get_qmi_message_id(buf, buf_len)));
        break; 
  }

  return 0;
}

void *register_to_mdmfs_service() {
  mdmfs_demo_read();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}