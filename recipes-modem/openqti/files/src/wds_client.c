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
#include <sys/socket.h>

#include "../inc/nas.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/qmi.h"
#include "../inc/wds.h"
#include <net/if.h>

/* 
Connection steps, as used in ModemManager:
  1. Load profile settings -> nah
  2. Open QMI port  -> already done 
  3. Setup data format -> check out wda service, do I really need to do this or is it ok with the defaults?
  4. Setup link
  5. Setup link main up
  6. Set IP method
  7. Allocate IPv4 WDS client
  8. Bind data port
  9. Set IP family
  10. Enable indications
  11. Start network

    CONNECT_STEP_FIRST,
    CONNECT_STEP_LOAD_PROFILE_SETTINGS,
    CONNECT_STEP_OPEN_QMI_PORT,
    CONNECT_STEP_SETUP_DATA_FORMAT,
    CONNECT_STEP_SETUP_LINK,
    CONNECT_STEP_SETUP_LINK_MAIN_UP,
    CONNECT_STEP_IP_METHOD,
    CONNECT_STEP_IPV4,
    CONNECT_STEP_WDS_CLIENT_IPV4,
    CONNECT_STEP_BIND_DATA_PORT_IPV4,
    CONNECT_STEP_IP_FAMILY_IPV4,
    CONNECT_STEP_ENABLE_INDICATIONS_IPV4,
    CONNECT_STEP_START_NETWORK_IPV4,
    CONNECT_STEP_ENABLE_WDS_INDICATIONS_IPV4, 01:10:00:00:01:03:00:04:00:03:00:04:00:12:01:00:01
    CONNECT_STEP_GET_CURRENT_SETTINGS_IPV4, 01:13:00:00:01:03:00:05:00:2D:00:07:00:10:04:00:30:E3:04:00
*/

struct {
  uint8_t curr_step;
  uint8_t mux_state;
  int pdp_session_handle; // we need this to stop the connection later
  uint32_t pkt_data_handle;
} wds_runtime;

void reset_wds_runtime() {
  wds_runtime.curr_step = 0;
  wds_runtime.mux_state = 0;
  wds_runtime.pdp_session_handle = -1;
  wds_runtime.pkt_data_handle = 0;
}

const char *get_wds_command(uint16_t msgid) {
  for (uint16_t i = 0;
       i < (sizeof(wds_svc_commands) / sizeof(wds_svc_commands[0])); i++) {
    if (wds_svc_commands[i].id == msgid) {
      return wds_svc_commands[i].cmd;
    }
  }
  return "WDS service: Unknown command\n";
}

int wds_sample_func() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       NAS_GET_CELL_LOCATION_INFO) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int wds_set_autoconnect(uint8_t enable) {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) + sizeof(struct qmi_generic_uint8_t_tlv);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       WDS_SET_AUTOCONNECT_SETTINGS) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  if (build_u8_tlv(pkt, pkt_len, curr_offset, 0x01, enable) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
    curr_offset += sizeof(struct qmi_generic_uint8_t_tlv);
  }
  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int wds_stop_network() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) + sizeof(struct qmi_generic_uint32_t_tlv);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       WDS_STOP_NETWORK) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  if (build_u32_tlv(pkt, pkt_len, curr_offset, 0x01, wds_runtime.pkt_data_handle) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
    curr_offset += sizeof(struct qmi_generic_uint32_t_tlv);
  }
  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int wds_enable_indications_ipv4() {
  size_t pkt_len = sizeof(struct qmux_packet) + sizeof(struct qmi_packet) + sizeof(struct qmi_generic_uint32_t_tlv);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       WDS_GET_CURRENT_SETTINGS) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  if (build_u32_tlv(pkt, pkt_len, curr_offset, 0x10, 0x0004e330) < 0) {
    logger(MSG_ERROR, "%s: Error adding the TLV\n", __func__);
    free(pkt);
    return -EINVAL;
    curr_offset += sizeof(struct qmi_generic_uint32_t_tlv);
  }
  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}

int set_rawip_mode() {
 int fd;
 struct ifreq *ifr = calloc(1, sizeof(struct ifreq));
  char netdev[] = "rmnet0";
  system("ifconfig rmnet0 down");
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    logger(MSG_ERROR, "%s: Can't open socket!\n", __func__);
    free(ifr);
    return -EBADFD;
  }

  strncpy(ifr->ifr_name, netdev, sizeof(ifr->ifr_name));

  if (ioctl(fd, RMNET_IOCTL_SET_LLP_IP, ifr) < 0)  {
    logger(MSG_ERROR, "%s: IOCTL Failed!\n", __func__);
    free(ifr);
    close(fd);
    return -EIO;
  }
  system("ifconfig rmnet0 169.252.10.1 netmask 255.0.0.0 allmulti multicast up");
  close(fd);
  free(ifr);
  return 0;

}

int wds_bind_mux_data_port() {
  size_t pkt_len = sizeof(struct qmux_packet) + 
                   sizeof(struct qmi_packet) + 
                   sizeof(struct ep_id) +
                   sizeof(struct mux_id);
  uint8_t *pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
    
  }
  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0,
                       WDS_BIND_MUX_DATA_PORT) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  size_t curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);
  struct ep_id *periph = (struct ep_id*)(pkt+curr_offset);
  periph->type = 0x10;
  periph->len = 0x08;
  periph->ep_type = DATA_EP_TYPE_BAM_DMUX;
  periph->interface_id = 0;

  curr_offset+= sizeof(struct ep_id);
  struct mux_id *mux = (struct mux_id *)(pkt+curr_offset);
  mux->type = 0x11;
  mux->len = 0x01;
  mux->mux_id = 0;
  
  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);
  free(pkt);
  return 0;
}


int wds_attempt_to_connect() {
  uint8_t *pkt = NULL;
  size_t curr_offset = 0;
  size_t pkt_len = sizeof(struct qmux_packet) + 
                   sizeof(struct qmi_packet);

  pkt_len+= sizeof(struct apn_config);
  pkt_len+= strlen(get_internal_network_apn_name());
  pkt_len+= sizeof(struct apn_type);

  if (get_internal_network_auth_method() != 0) {
    pkt_len+= sizeof(struct wds_auth_pref);
    pkt_len+= sizeof(struct wds_auth_username);
    pkt_len+= sizeof(struct wds_auth_password);
    pkt_len+= strlen(get_internal_network_username());
    pkt_len+= strlen(get_internal_network_pass());
  }
  
  pkt_len+= sizeof(struct ip_family_preference);
  pkt_len+= sizeof(struct call_type);
  pkt_len+= sizeof(struct block_in_roaming);

  pkt = malloc(pkt_len);
  memset(pkt, 0, pkt_len);

  if (build_qmux_header(pkt, pkt_len, 0x00, QMI_SERVICE_WDS, 0) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmux header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  if (build_qmi_header(pkt, pkt_len, QMI_REQUEST, 0, WDS_START_NETWORK) < 0) {
    logger(MSG_ERROR, "%s: Error adding the qmi header\n", __func__);
    free(pkt);
    return -EINVAL;
  }

  curr_offset = sizeof(struct qmux_packet) + sizeof(struct qmi_packet);

  struct apn_config *apn = (struct apn_config*)(pkt+curr_offset);
  apn->type = 0x14;
  apn->len = strlen(get_internal_network_apn_name());
  memcpy(apn->apn, get_internal_network_apn_name(), apn->len);
  curr_offset+= sizeof(struct apn_config) + apn->len;

  if (get_internal_network_auth_method() != 0) {
    struct wds_auth_pref *auth_pref = (struct wds_auth_pref *)(pkt+curr_offset);
    auth_pref->type = 0x16;
    auth_pref->len = 0x01;
    auth_pref->preference = get_internal_network_auth_method();
    curr_offset+= sizeof(struct wds_auth_pref);

    struct wds_auth_username *user = (struct wds_auth_username *)(pkt+curr_offset);
    user->type = 0x17;
    user->len = strlen(get_internal_network_username());
    memcpy(user->username, get_internal_network_username(), user->len);
    curr_offset+= sizeof(struct wds_auth_username) + user->len;

    struct wds_auth_password *pass = (struct wds_auth_password *)(pkt+curr_offset);
    pass->type = 0x18;
    pass->len = strlen(get_internal_network_pass());
    memcpy(pass->password, get_internal_network_pass(), pass->len);
    curr_offset+= sizeof(struct wds_auth_password) + pass->len;
  }

  struct ip_family_preference *ip_family = (struct ip_family_preference *)(pkt+curr_offset);
  ip_family->type = 0x19;
  ip_family->len = 0x01;
  ip_family->value = 4;
  curr_offset+= sizeof(struct ip_family_preference);

  struct call_type *ctype = (struct call_type *)(pkt+curr_offset);
  ctype->type = 0x35;
  ctype->len = 0x01;
  ctype->value = WDS_CALL_TYPE_EMBEDDED; // 0x00:LAPTOP, 0x01:embedded
  curr_offset+= sizeof(struct call_type);

  struct apn_type *apn_type = (struct apn_type *)(pkt+curr_offset);
  apn_type->type = 0x38;
  apn_type->len = 0x04;
  apn_type->value = htole32(WDS_APN_TYPE_INTERNET);
  curr_offset+= sizeof(struct apn_type);

  struct block_in_roaming *roaming_lock = (struct block_in_roaming *)(pkt+curr_offset);
  roaming_lock->type = 0x39;
  roaming_lock->len = 0x01;
  roaming_lock->value = 0x00; // OFF
  curr_offset+= sizeof(struct block_in_roaming);

  add_pending_message(QMI_SERVICE_WDS, (uint8_t *)pkt, pkt_len);

  free(pkt);
  return 0;
}

void *init_internal_networking() {
  if (!is_internal_connect_enabled()) {
    logger(MSG_WARN, "%s: Internal networking is disabled\n", __func__);
    return NULL;
  }

  while (!get_network_type()) {
    logger(MSG_INFO, "[%s] Waiting for network to be ready...\n", __func__);
    sleep(10);
  }

  logger(MSG_INFO, "%s: WDS: Network is ready, try to connect\n", __func__);

  wds_set_autoconnect(0);
  set_rawip_mode();

  wds_attempt_to_connect();

  return NULL;
}

#define DEBUG_WDS 1
/*
 * Reroutes messages from the internal QMI client to the service
 */
int handle_incoming_wds_message(uint8_t *buf, size_t buf_len) {
  logger(MSG_INFO, "%s: Start\n", __func__);
  uint16_t qmi_err;

#ifdef DEBUG_WDS
  pretty_print_qmi_pkt("WDS: Baseband --> Host", buf, buf_len);
#endif
  switch (get_qmi_message_id(buf, buf_len)) {
  case WDS_BIND_MUX_DATA_PORT:
      qmi_err = did_qmi_op_fail(buf, buf_len);
      if (qmi_err == QMI_RESULT_FAILURE) {
        logger(MSG_ERROR, "%s failed to bind the port\n", __func__);
        wds_runtime.mux_state = 0;
      } else if (qmi_err == QMI_RESULT_SUCCESS) {
        logger(MSG_INFO, "%s succeeded to bind the port\n", __func__);
        wds_runtime.mux_state = 2;
      } else if (qmi_err == QMI_RESULT_UNKNOWN) {
        logger(MSG_ERROR, "%s: QMI message didn't have an indication\n",
               __func__);
      }
    break;
  case WDS_START_NETWORK:
    if (did_qmi_op_fail(buf, buf_len)) {
      logger(MSG_ERROR, "%s failed to start network\n", __func__);
    } else {
      logger(MSG_INFO, "%s: Network started! enable indications and request dhcp\n", __func__);
      system("udhcpc -q -f -i rmnet0");
    }
    wds_enable_indications_ipv4();
    break;
  case WDS_EXTENDED_ERROR_CODE:
    logger(MSG_INFO, "%s: Extended Error Code\n", __func__);
    break;
  case WDS_PROFILE_IDENTIFIER:
    logger(MSG_INFO, "%s: Profile Identifier\n", __func__);
    break;
  case WDS_PROFILE_NAME:
    logger(MSG_INFO, "%s: Profile Name\n", __func__);
    break;
  case WDS_PDP_TYPE:
    logger(MSG_INFO, "%s: PDP Type\n", __func__);
    break;
  case WDS_PDP_HEADER_COMPRESSION_TYPE:
    logger(MSG_INFO, "%s: PDP Header Compression Type\n", __func__);
    break;
  case WDS_PDP_DATA_COMPRESSION_TYPE:
    logger(MSG_INFO, "%s: PDP Data Compression Type\n", __func__);
    break;
  case WDS_APN_NAME:
    logger(MSG_INFO, "%s: APN Name\n", __func__);
    break;
  case WDS_PRIMARY_IPV4_DNS_ADDRESS:
    logger(MSG_INFO, "%s: Primary IPv4 DNS Address\n", __func__);
    break;
  case WDS_SECONDARY_IPV4_DNS_ADDRESS:
    logger(MSG_INFO, "%s: Secondary IPv4 DNS Address\n", __func__);
    break;
  case WDS_UMTS_REQUESTED_QOS:
    logger(MSG_INFO, "%s: UMTS Requested QoS\n", __func__);
    break;
  case WDS_UMTS_MINIMUM_QOS:
    logger(MSG_INFO, "%s: UMTS Minimum QoS\n", __func__);
    break;
  case WDS_GPRS_REQUESTED_QOS:
    logger(MSG_INFO, "%s: GPRS Requested QoS\n", __func__);
    break;
  case WDS_GPRS_MINIMUM_QOS:
    logger(MSG_INFO, "%s: GPRS Minimum QoS\n", __func__);
    break;
  case WDS_USERNAME:
    logger(MSG_INFO, "%s: Username\n", __func__);
    break;
  case WDS_PASSWORD:
    logger(MSG_INFO, "%s: Password\n", __func__);
    break;
  case WDS_AUTHENTICATION:
    logger(MSG_INFO, "%s: Authentication\n", __func__);
    break;
  case WDS_LTE_QOS_PARAMETERS:
    logger(MSG_INFO, "%s: LTE QoS Parameters\n", __func__);
    break;
  case WDS_APN_DISABLED_FLAG:
    logger(MSG_INFO, "%s: APN Disabled Flag\n", __func__);
    break;
  case WDS_ROAMING_DISALLOWED_FLAG:
    logger(MSG_INFO, "%s: Roaming Disallowed Flag\n", __func__);
    break;
  case WDS_APN_TYPE_MASK:
    logger(MSG_INFO, "%s: APN Type Mask\n", __func__);
    break;
  case WDS_RESET:
    logger(MSG_INFO, "%s: Reset\n", __func__);
    break;
  case WDS_ABORT:
    logger(MSG_INFO, "%s: Abort\n", __func__);
    break;
  case WDS_INDICATION_REGISTER:
    logger(MSG_INFO, "%s: Indication Register\n", __func__);
    break;
  case WDS_STOP_NETWORK:
    logger(MSG_INFO, "%s: Stop Network\n", __func__);
    break;
  case WDS_GET_PACKET_SERVICE_STATUS:
    logger(MSG_INFO, "%s: Get Packet Service Status\n", __func__);
    break;
  case WDS_GET_CHANNEL_RATES:
    logger(MSG_INFO, "%s: Get Channel Rates\n", __func__);
    break;
  case WDS_GET_PACKET_STATISTICS:
    logger(MSG_INFO, "%s: Get Packet Statistics\n", __func__);
    break;
  case WDS_GO_DORMANT:
    logger(MSG_INFO, "%s: Go Dormant\n", __func__);
    break;
  case WDS_GO_ACTIVE:
    logger(MSG_INFO, "%s: Go Active\n", __func__);
    break;
  case WDS_CREATE_PROFILE:
    logger(MSG_INFO, "%s: Create Profile\n", __func__);
    break;
  case WDS_MODIFY_PROFILE:
    logger(MSG_INFO, "%s: Modify Profile\n", __func__);
    break;
  case WDS_DELETE_PROFILE:
    logger(MSG_INFO, "%s: Delete Profile\n", __func__);
    break;
  case WDS_GET_PROFILE_LIST:
    logger(MSG_INFO, "%s: Get Profile List\n", __func__);
    break;
  case WDS_GET_PROFILE_SETTINGS:
    logger(MSG_INFO, "%s: Get Profile Settings\n", __func__);
    break;
  case WDS_GET_DEFAULT_SETTINGS:
    logger(MSG_INFO, "%s: Get Default Settings\n", __func__);
    break;
  case WDS_GET_CURRENT_SETTINGS:
    logger(MSG_INFO, "%s: Get Current Settings\n", __func__);
    break;
  case WDS_GET_DORMANCY_STATUS:
    logger(MSG_INFO, "%s: Get Dormancy Status\n", __func__);
    break;
  case WDS_GET_AUTOCONNECT_SETTINGS:
    logger(MSG_INFO, "%s: Get Autoconnect Settings\n", __func__);
    break;
  case WDS_GET_DATA_BEARER_TECHNOLOGY:
    logger(MSG_INFO, "%s: Get Data Bearer Technology\n", __func__);
    break;
  case WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY:
    logger(MSG_INFO, "%s: Get Current Data Bearer Technology\n", __func__);
    break;
  case WDS_GET_DEFAULT_PROFILE_NUMBER:
    logger(MSG_INFO, "%s: Get Default Profile Number\n", __func__);
    break;
  case WDS_SET_DEFAULT_PROFILE_NUMBER:
    logger(MSG_INFO, "%s: Set Default Profile Number\n", __func__);
    break;
  case WDS_SET_IP_FAMILY:
    logger(MSG_INFO, "%s: Set IP Family\n", __func__);
    break;
  case WDS_SET_AUTOCONNECT_SETTINGS:
    logger(MSG_INFO, "%s: Set Autoconnect Settings\n", __func__);
    break;
  case WDS_GET_PDN_THROTTLE_INFO:
    logger(MSG_INFO, "%s: Get PDN Throttle Info\n", __func__);
    break;
  case WDS_GET_LTE_ATTACH_PARAMETERS:
    logger(MSG_INFO, "%s: Get LTE Attach Parameters\n", __func__);
    break;
  case WDS_BIND_DATA_PORT:
    logger(MSG_INFO, "%s: Bind Data Port\n", __func__);
    break;
  case WDS_EXTENDED_IP_CONFIG:
    logger(MSG_INFO, "%s: Extended Ip Config\n", __func__);
    break;
  case WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER:
    logger(MSG_INFO, "%s: Get Max LTE Attach PDN Number\n", __func__);
    break;
  case WDS_SET_LTE_ATTACH_PDN_LIST:
    logger(MSG_INFO, "%s: Set LTE Attach PDN List\n", __func__);
    break;
  case WDS_GET_LTE_ATTACH_PDN_LIST:
    logger(MSG_INFO, "%s: Get LTE Attach PDN List\n", __func__);
    break;
  case WDS_CONFIGURE_PROFILE_EVENT_LIST:
    logger(MSG_INFO, "%s: Configure Profile Event List\n", __func__);
    break;
  case WDS_PROFILE_CHANGED:
    logger(MSG_INFO, "%s: Profile Changed\n", __func__);
    break;
  case WDS_SWI_CREATE_PROFILE_INDEXED:
    logger(MSG_INFO, "%s: Swi Create Profile Indexed\n", __func__);
    break;
  default:
    logger(MSG_INFO, "%s: Unhandled message for WDS: %.4x\n", __func__,
           get_qmi_message_id(buf, buf_len));
    break;
  }

  return 0;
}