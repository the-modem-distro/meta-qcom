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

#include "../inc/pdc.h"
#include "config.h"
#include "devices.h"
#include "ipc.h"
#include "logger.h"
#include "qmi.h"

#define DEBUG_PDC 1

const char *get_pdc_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(pdc_svc_commands) / sizeof(pdc_svc_commands[0])); i++) {
    if (pdc_svc_commands[i].id == msgid) {
      return pdc_svc_commands[i].cmd;
    }
  }
  return "PDC: Unknown command\n";
}

int pdc_register_to_events() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   (2 * sizeof(struct qmi_generic_uint8_t_tlv));
  uint8_t *pkt = malloc(pkt_len);
  uint8_t tlvs[] = {0x10, 0x11};
  memset(pkt, 0, pkt_len);
  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_PDC, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       PERSISTENT_DEVICE_CONFIG_INDICATION_REGISTER) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  for (int i = 0; i < 2; i++) {

    if (build_u8_tlv(pkt, pkt_len, curr_offset, tlvs[i], 1) < 0) {
      logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
      free(pkt);
      return -EINVAL;
    }
    curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  }

  add_pending_message(QMI_SERVICE_PDC, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}


int pdc_request_config_list() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) +
                   sizeof(struct pdc_indication_key) + sizeof(struct pdc_setting_type);
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
  struct pdc_indication_key *key = (struct pdc_indication_key *)(pkt + curr_offset);
  key->id = 0x10;
  key->len = sizeof(uint32_t);
  key->key = 0xdeadbeef;

  curr_offset += sizeof(struct pdc_indication_key);
  struct pdc_setting_type *config_type = (struct pdc_setting_type *)(pkt + curr_offset);
  config_type->id = 0x11;
  config_type->len = sizeof(uint32_t);
  config_type->data = PDC_CONFIG_SW;

  add_pending_message(QMI_SERVICE_PDC, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}
int pdc_read_config_list(uint8_t *buf, size_t buf_len) {
    uint16_t qmi_err = did_qmi_op_fail(buf, buf_len);
    if (qmi_err == QMI_RESULT_FAILURE) {
      logger(MSG_ERROR, "%s Get config failed!\n", __func__);
    } else if (qmi_err == QMI_RESULT_SUCCESS) {
      logger(MSG_INFO, "%s succeeded\n", __func__);
    } else if (qmi_err == QMI_RESULT_UNKNOWN) {
      logger(MSG_ERROR, "%s: QMI message didn't have an indication\n",
             __func__);
    }

    int offset = get_tlv_offset_by_id(buf, buf_len, 0x11);
    if (offset > 0) {
        // Do something here
        struct pdc_config_list *config_list = (struct pdc_config_list*)(buf + offset);
        size_t pktoffset = offset + sizeof(struct pdc_config_list); // we start at the first element
        for (uint8_t i = 0; i < config_list->num_elements; i++) {
            struct pdc_config_item_header *item = (struct pdc_config_item_header *)(buf + pktoffset);
            logger(MSG_INFO, "%s: Item:\n"
                              " - Config type: %.2x\n"
                              " - Config size: %u\n"
                              " - Config ID: %x\n",
                              __func__,
                              item->config_type,
                              item->config_id_size,
                              item->config_id);
            pktoffset+= sizeof(struct pdc_config_item_header) + item->config_id_size;
            if (pktoffset > buf_len) {
                logger(MSG_ERROR, "%s: Whooops we're reading outside of the message size!\n", __func__);
                return -ENOMEM;
            }
        
        
        }
    }
    return 0;
}
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_pdc_message(uint8_t *buf, size_t buf_len) {
   logger(MSG_INFO, "%s: Start: Message from the PDC Service: %.4x | %s\n",
           __func__, get_qmi_message_id(buf, buf_len), get_pdc_command(get_qmi_message_id(buf, buf_len)));

#ifdef DEBUG_PDC
  pretty_print_qmi_pkt("PDC: Baseband --> Host", buf, buf_len);
#endif

  switch (get_qmi_message_id(buf, buf_len)) {
    case PERSISTENT_DEVICE_CONFIG_RESET:
    case PERSISTENT_DEVICE_CONFIG_GET_SUPPORTED_MSGS:
    case PERSISTENT_DEVICE_CONFIG_GET_SUPPORTED_FIELDS:
    break;
    case PERSISTENT_DEVICE_CONFIG_INDICATION_REGISTER:
        pdc_request_config_list();
        break;
    case PERSISTENT_DEVICE_CONFIG_CONFIG_CHANGE_IND:
    case PERSISTENT_DEVICE_CONFIG_GET_SELECTED_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_SET_SELECTED_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_LIST_CONFIGS:
        pdc_read_config_list(buf, buf_len);
        break;
    case PERSISTENT_DEVICE_CONFIG_DELETE_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_LOAD_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_ACTIVATE_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_GET_CONFIG_INFO:
    case PERSISTENT_DEVICE_CONFIG_GET_CONFIG_LIMITS:
    case PERSISTENT_DEVICE_CONFIG_GET_DEFAULT_CONFIG_INFO:
    case PERSISTENT_DEVICE_CONFIG_DEACTIVATE_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_VALIDATE_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_GET_FEATURE:
    case PERSISTENT_DEVICE_CONFIG_SET_FEATURE:
    case PERSISTENT_DEVICE_CONFIG_REFRESH_IND:
    case PERSISTENT_DEVICE_CONFIG_GET_CONFIG:
    case PERSISTENT_DEVICE_CONFIG_NOTIFICATION:
    default:
        logger(MSG_INFO, "%s: Unhandled message for the PDC Service: %.4x | %s\n",
            __func__, get_qmi_message_id(buf, buf_len), get_pdc_command(get_qmi_message_id(buf, buf_len)));
        break; 
  }

  return 0;
}

void *register_to_pdc_service() {
  pdc_register_to_events();
  logger(MSG_INFO, "%s finished!\n", __func__);
  return NULL;
}