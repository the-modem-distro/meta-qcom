/* SPDX-License-Identifier: MIT */

#ifndef _QMI_H
#define _QMI_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define QMI_RESULT_SUCCESS 0x0000
#define QMI_RESULT_FAILURE 0x0001
#define QMI_RESULT_UNKNOWN 0x0002

static const struct {
  uint16_t code;
  const char *error_name;
} qmi_error_codes[] = {
    {0x0000, "none"},
    {0x0001, "malformed_msg"},
    {0x0002, "no_memory"},
    {0x0003, "internal"},
    {0x0004, "aborted"},
    {0x0005, "client_ids_exhausted"},
    {0x0006, "unabortable_transaction"},
    {0x0007, "invalid_client_id"},
    {0x0008, "no_thresholds"},
    {0x0009, "invalid_handle"},
    {0x000A, "invalid_profile"},
    {0x000B, "invalid_pinid"},
    {0x000C, "incorrect_pin"},
    {0x000D, "no_network_found"},
    {0x000E, "call_failed"},
    {0x000F, "out_of_call"},
    {0x0010, "not_provisioned"},
    {0x0011, "missing_arg"},
    {0x0013, "arg_too_long"},
    {0x0016, "invalid_tx_id"},
    {0x0017, "device_in_use"},
    {0x0018, "op_network_unsupported"},
    {0x0019, "op_device_unsupported"},
    {0x001A, "no_effect"},
    {0x001B, "no_free_profile"},
    {0x001C, "invalid_pdp_type"},
    {0x001D, "invalid_tech_pref"},
    {0x001E, "invalid_profile_type"},
    {0x001F, "invalid_service_type"},
    {0x0020, "invalid_register_action"},
    {0x0021, "invalid_ps_attach_action"},
    {0x0022, "authentication_failed"},
    {0x0023, "pin_blocked"},
    {0x0024, "pin_perm_blocked"},
    {0x0025, "sim_not_initialized"},
    {0x0026, "max_qos_requests_in_use"},
    {0x0027, "incorrect_flow_filter"},
    {0x0028, "network_qos_unaware"},
    {0x0029, "invalid_id"},
    {0x0029, "invalid_qos_id"},
    {0x002A, "requested_num_unsupported"},
    {0x002B, "interface_not_found"},
    {0x002C, "flow_suspended"},
    {0x002D, "invalid_data_format"},
    {0x002E, "general"},
    {0x002F, "unknown"},
    {0x0030, "invalid_arg"},
    {0x0031, "invalid_index"},
    {0x0032, "no_entry"},
    {0x0033, "device_storage_full"},
    {0x0034, "device_not_ready"},
    {0x0035, "network_not_ready"},
    {0x0036, "cause_code"},
    {0x0037, "message_not_sent"},
    {0x0038, "message_delivery_failure"},
    {0x0039, "invalid_message_id"},
    {0x003A, "encoding"},
    {0x003B, "authentication_lock"},
    {0x003C, "invalid_transition"},
    {0x003D, "not_a_mcast_iface"},
    {0x003E, "max_mcast_requests_in_use"},
    {0x003F, "invalid_mcast_handle"},
    {0x0040, "invalid_ip_family_pref"},
    {0x0041, "session_inactive"},
    {0x0042, "session_invalid"},
    {0x0043, "session_ownership"},
    {0x0044, "insufficient_resources"},
    {0x0045, "disabled"},
    {0x0046, "invalid_operation"},
    {0x0047, "invalid_qmi_cmd"},
    {0x0048, "tpdu_type"},
    {0x0049, "smsc_addr"},
    {0x004A, "info_unavailable"},
    {0x004B, "segment_too_long"},
    {0x004C, "segment_order"},
    {0x004D, "bundling_not_supported"},
    {0x004E, "op_partial_failure"},
    {0x004F, "policy_mismatch"},
    {0x0050, "sim_file_not_found"},
    {0x0051, "extended_internal"},
    {0x0052, "access_denied"},
    {0x0053, "hardware_restricted"},
    {0x0054, "ack_not_sent"},
    {0x0055, "inject_timeout"},
    {0x005A, "incompatible_state"},
    {0x005B, "fdn_restrict"},
    {0x005C, "sups_failure_cause"},
    {0x005D, "no_radio"},
    {0x005E, "not_supported"},
    {0x005F, "no_subscription"},
    {0x0060, "card_call_control_failed"},
    {0x0061, "network_aborted"},
    {0x0062, "msg_blocked"},
    {0x0064, "invalid_session_type"},
    {0x0065, "invalid_pb_type"},
    {0x0066, "no_sim"},
    {0x0067, "pb_not_ready"},
    {0x0068, "pin_restriction"},
    {0x0069, "pin2_restriction"},
    {0x006A, "puk_restriction"},
    {0x006B, "puk2_restriction"},
    {0x006C, "pb_access_restricted"},
    {0x006D, "pb_delete_in_prog"},
    {0x006E, "pb_text_too_long"},
    {0x006F, "pb_number_too_long"},
    {0x0070, "pb_hidden_key_restriction"},
    {0x0071, "pb_not_available"},
    {0x0072, "device_memory_error"},
    {0x0073, "no_permission"},
    {0x0074, "too_soon"},
    {0x0075, "time_not_acquired"},
    {0x0076, "op_in_progress"},
    {0x0101, "eperm"},
    {0x0102, "enoent"},
    {0x0103, "esrch"},
    {0x0104, "eintr"},
    {0x0105, "eio"},
    {0x0106, "enxio"},
    {0x0107, "e2big"},
    {0x0108, "enoexec"},
    {0x0109, "ebadf"},
    {0x010A, "echild"},
    {0x010B, "eagain"},
    {0x010C, "enomem"},
    {0x010D, "eacces"},
    {0x010E, "efault"},
    {0x010F, "enotblk"},
    {0x0110, "ebusy"},
    {0x0111, "eexist"},
    {0x0112, "exdev"},
    {0x0113, "enodev"},
    {0x0114, "enotdir"},
    {0x0115, "eisdir"},
    {0x0116, "einval"},
    {0x0117, "enfile"},
    {0x0118, "emfile"},
    {0x0119, "enotty"},
    {0x011A, "etxtbsy"},
    {0x011B, "efbig"},
    {0x011C, "enospc"},
    {0x011D, "espipe"},
    {0x011E, "erofs"},
    {0x011F, "emlink"},
    {0x0120, "epipe"},
    {0x0121, "edom"},
    {0x0122, "erange"},
    {0x0123, "edeadlk"},
    {0x0124, "enametoolong"},
    {0x0125, "enolck"},
    {0x0126, "enosys"},
    {0x0127, "enotempty"},
    {0x0128, "eloop"},
    {0x010B, "ewouldblock"},
    {0x012A, "enomsg"},
    {0x012B, "eidrm"},
    {0x012C, "echrng"},
    {0x012D, "el2nsync"},
    {0x012E, "el3hlt"},
    {0x012F, "el3rst"},
    {0x0130, "elnrng"},
    {0x0131, "eunatch"},
    {0x0132, "enocsi"},
    {0x0133, "el2hlt"},
    {0x0134, "ebade"},
    {0x0135, "ebadr"},
    {0x0136, "exfull"},
    {0x0137, "enoano"},
    {0x0138, "ebadrqc"},
    {0x0139, "ebadslt"},
    {0x0123, "edeadlock"},
    {0x013B, "ebfont"},
    {0x013C, "enostr"},
    {0x013D, "enodata"},
    {0x013E, "etime"},
    {0x013F, "enosr"},
    {0x0140, "enonet"},
    {0x0141, "enopkg"},
    {0x0142, "eremote"},
    {0x0143, "enolink"},
    {0x0144, "eadv"},
    {0x0145, "esrmnt"},
    {0x0146, "ecomm"},
    {0x0147, "eproto"},
    {0x0148, "emultihop"},
    {0x0149, "edotdot"},
    {0x014A, "ebadmsg"},
    {0x014B, "eoverflow"},
    {0x014C, "enotuniq"},
    {0x014D, "ebadfd"},
    {0x014E, "eremchg"},
    {0x014F, "elibacc"},
    {0x0150, "elibbad"},
    {0x0151, "elibscn"},
    {0x0152, "elibmax"},
    {0x0153, "elibexec"},
    {0x0154, "eilseq"},
    {0x0155, "erestart"},
    {0x0156, "estrpipe"},
    {0x0157, "eusers"},
    {0x0158, "enotsock"},
    {0x0159, "edestaddrreq"},
    {0x015A, "emsgsize"},
    {0x015B, "eprototype"},
    {0x015C, "enoprotoopt"},
    {0x015D, "eprotonosupport"},
    {0x015E, "esocktnosupport"},
    {0x015F, "eopnotsupp"},
    {0x0160, "epfnosupport"},
    {0x0161, "eafnosupport"},
    {0x0162, "eaddrinuse"},
    {0x0163, "eaddrnotavail"},
    {0x0164, "enetdown"},
    {0x0165, "enetunreach"},
    {0x0166, "enetreset"},
    {0x0167, "econnaborted"},
    {0x0168, "econnreset"},
    {0x0169, "enobufs"},
    {0x016A, "eisconn"},
    {0x016B, "enotconn"},
    {0x016C, "eshutdown"},
    {0x016D, "etoomanyrefs"},
    {0x016E, "etimedout"},
    {0x016F, "econnrefused"},
    {0x0170, "ehostdown"},
    {0x0171, "ehostunreach"},
    {0x0172, "ealready"},
    {0x0173, "einprogress"},
    {0x0174, "estale"},
    {0x0175, "euclean"},
    {0x0176, "enotnam"},
    {0x0177, "enavail"},
    {0x0178, "eisnam"},
    {0x0179, "eremoteio"},
    {0x017A, "edquot"},
    {0x017B, "enomedium"},
    {0x017C, "emediumtype"},
    {0x017D, "ecanceled"},
    {0x017E, "enokey"},
    {0x017F, "ekeyexpired"},
    {0x0180, "ekeyrevoked"},
    {0x0181, "ekeyrejected"},
    {0x0182, "eownerdead"},
    {0x0183, "enotrecoverable"},
};

typedef enum {
  QMI_REQUEST = 0x00,
  QMI_RESPONSE = 0x01,
  QMI_INDICATION = 0x02,
  QMI_RESERVED = 0x03,
} QMI_Control;

typedef enum {
  QMI_SERVICE_CONTROL = 0x00,
  QMI_SERVICE_WDS = 0x01,
  QMI_SERVICE_DMS = 0x02,
  QMI_SERVICE_NAS = 0x03,
  QMI_SERVICE_QOS = 0x04,
  QMI_SERVICE_WMS = 0x05,
  QMI_SERVICE_PDS = 0x06,
  QMI_SERVICE_AUTH = 0x07,
  QMI_SERVICE_AT = 0x08,
  QMI_SERVICE_VOICE = 0x09,
  QMI_SERVICE_SAT = 0x0a, // SIM Application Toolkit
  QMI_SERVICE_UIM = 0x0b,
  QMI_SERVICE_PBOOK = 0x0c,
  QMI_SERVICE_QCHAT = 0x0d,
  QMI_SERVICE_RFS = 0x0e,
  QMI_SERVICE_TEST = 0x0f,
  QMI_SERVICE_LOCATION = 0x10,
  QMI_SERVICE_SAR = 0x11,
  QMI_SERVICE_IMS_SETTINGS = 0x12,
  QMI_SERVICE_ADCD = 0x13,
  QMI_SERVICE_SOUND = 0x14,
  QMI_SERVICE_MDMFS = 0x15,
  QMI_SERVICE_TIME = 0x16,
  QMI_SERVICE_THERMAL_SENSORS = 0x17,
  QMI_SERVICE_THERMAL_MITIGATION = 0x18,
  QMI_SERVICE_SAP = 0x19, // Service Access Proxy  
  QMI_SERVICE_PDC = 0x24, // dec36, ipch
  QMI_SERVICES_LAST = 0xff,
} QMI_Service;

enum {
	 CONTROL_SERVICE_SET_INSTANCE_ID = 0x0020, 
	 CONTROL_SERVICE_GET_VERSION_INFO = 0x0021, 
	 CONTROL_SERVICE_GET_CLIENT_ID = 0x0022, 
	 CONTROL_SERVICE_RELEASE_CLIENT_ID = 0x0023, 
	 CONTROL_SERVICE_REVOKE_CLIENT_ID_IND = 0x0024, 
	 CONTROL_SERVICE_INVALID_CLIENT_ID_IND = 0x0025, 
	 CONTROL_SERVICE_SET_DATA_FORMAT = 0x0026, 
	 CONTROL_SERVICE_SYNC = 0x0027, 
	 CONTROL_SERVICE_SYNC_IND = 0x0027, 
	 CONTROL_SERVICE_REG_PWR_SAVE_MODE = 0x0028, 
	 CONTROL_SERVICE_PWR_SAVE_MODE_IND = 0x0028, 
	 CONTROL_SERVICE_CONFIG_PWR_SAVE_SETTINGS = 0x0029, 
	 CONTROL_SERVICE_SET_PWR_SAVE_MODE = 0x002A, 
	 CONTROL_SERVICE_GET_PWR_SAVE_MODE = 0x002B, 
	 CONTROL_SERVICE_CONFIGURE_RESPONSE_FILTERING_IN_PWR_SAVE = 0x002C, 
	 CONTROL_SERVICE_GET_RESPONSE_FILTERING_SETTING_IN_PWR_SAVE = 0x002D, 
	 CONTROL_SERVICE_SET_SVC_AVAIL_LIST = 0x002E, 
	 CONTROL_SERVICE_GET_SVC_AVAIL_LIST = 0x002F, 
	 CONTROL_SERVICE_SET_EVENT_REPORT = 0x0030, 
	 CONTROL_SERVICE_SVC_AVAIL_IND = 0x0031, 
	 CONTROL_SERVICE_CONFIG_PWR_SAVE_SETTINGS_EXT = 0x0032, 
};

struct qmi_service_bindings {
  uint8_t service;
  uint8_t instance;
  uint8_t is_initialized;
  uint8_t has_pending_message;
  uint8_t *message;
  size_t message_len;
  uint16_t transaction_id;
};

struct qmux_packet {      // 6 byte
  uint8_t version;        // 0x01 ?? it's always 0x01, no idea what it is
  uint16_t packet_length; // Size of the entire message - sizeof (uint8_t) <-- version
  uint8_t control;        // 0x80 | 0x00
  uint8_t service;        // Service (WDS, IMS, Voice, ...)
  uint8_t instance_id; // Client ID
} __attribute__((packed));

/* "Normal QMI message, QMUX header indicates the service this message is for "*/
struct qmi_packet { // 7 byte
  uint8_t ctlid;    // 0x00 | 0x02 | 0x04, subsystem inside message service?
  uint16_t transaction_id; // QMI Transaction ID
  uint16_t msgid;          // QMI Message ID
  uint16_t length;         // QMI Packet size
} __attribute__((packed));

/* QMUX header with no service, to control the QMI service itself*/
struct ctl_qmi_packet { // 6 Byte
  uint8_t ctlid;
  uint8_t transaction_id;
  uint16_t msgid;
  uint16_t length;
} __attribute__((packed));

struct qmi_generic_result_ind {
  uint8_t result_code_type;     // 0x02
  uint16_t generic_result_size; // 0x04 0x00
  uint16_t result;
  uint16_t response;
} __attribute__((packed));

struct qmi_generic_uch_arr {
  uint8_t id;
  uint16_t len;
  uint8_t *data[0];
} __attribute__((packed));


struct qmi_generic_uint8_t_tlv {
  uint8_t id;
  uint16_t len; //1
  uint8_t data;
} __attribute__((packed));

struct qmi_generic_uint16_t_tlv {
  uint8_t id;
  uint16_t len; //2
  uint16_t data;
} __attribute__((packed));

struct qmi_generic_uint32_t_tlv {
  uint8_t id;
  uint16_t len; //4
  uint32_t data;
} __attribute__((packed));

struct encapsulated_qmi_packet {
  struct qmux_packet qmux;
  struct qmi_packet qmi;
} __attribute__((packed));

struct tlv_header {
  uint8_t id;   // 0x00
  uint16_t len; // 0x02 0x00
} __attribute__((packed));

struct signal_quality_tlv {
  uint8_t id;   // 0x10 = CDMA, 0x11 HDR, 0x12 GSM, 0x13 WCDMA, 0x14 LTE
  uint16_t len; // We only care about the RSSI, but there's more stuff in here
  uint8_t signal_level; // RSSI
} __attribute__((packed));

struct nas_signal_lev {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Operation result */
  struct qmi_generic_result_ind result;
  /* Signal level data */
  struct signal_quality_tlv signal;

} __attribute__((packed));

enum {
  PACKET_EMPTY = 0,
  PACKET_PASS_TRHU,
  PACKET_BYPASS,
  PACKET_FORCED_PT,
};

struct empty_tlv {
  uint8_t id;
  uint16_t len;
  uint8_t data[0];
} __attribute__((packed));

struct tlv_position {
  uint8_t id;
  uint32_t offset;
  uint16_t size;
};

struct qmux_alloc_pkt {   // 7 byte
  uint8_t version;        // 0x01 ?? it's always 0x01, no idea what it is
  uint16_t packet_length; // sz
  uint16_t service;
  uint16_t instance_id;
} __attribute__((packed));

/* Client alloc */
struct qmi_ctl_alloc_request {
  uint8_t id; //
  uint16_t msgid;
  uint16_t size;
} __attribute__((packed));

struct qmi_ctl_service {
  uint8_t id;         // 0x01
  uint16_t len;       // 1 byte
  uint8_t service_id; // Service ID
} __attribute__((packed));

struct qmi_service_instance_pair {
  uint8_t id;          // 0x01
  uint16_t len;        // 2 byte
  uint8_t service_id;  // Service ID
  uint8_t instance_id; // Client ID
} __attribute__((packed));

struct client_alloc_request {
  struct qmux_alloc_pkt qmux;
  struct qmi_ctl_alloc_request qmi;
  struct qmi_ctl_service service;
} __attribute__((packed));

struct client_alloc_response {
  struct qmux_packet qmux;
  struct ctl_qmi_packet qmi;
  struct qmi_generic_result_ind result;
  struct qmi_service_instance_pair instance;
} __attribute__((packed));

/* Service availability */
struct svc_avail_list {
  uint8_t id; // 0x01
  uint16_t len; // 255
  uint8_t svc[32]; // We're going to request all of them
} __attribute__((packed));

struct qmi_ctl_set_service_availability {
  struct qmux_alloc_pkt qmux;
  struct qmi_ctl_alloc_request qmi;
  struct svc_avail_list service;
} __attribute__((packed));

struct qmi_ctl_service_availability_request {
  struct qmux_alloc_pkt qmux;
  struct qmi_ctl_alloc_request qmi;
} __attribute__((packed));

uint8_t get_qmux_service_id(void *bytes, size_t len);
uint8_t get_qmux_instance_id(void *bytes, size_t len);
uint16_t get_control_message_id(void *bytes, size_t len);
uint16_t get_qmi_message_id(void *bytes, size_t len);
uint16_t get_qmi_message_type(void *bytes, size_t len);
uint16_t get_qmi_transaction_id(void *bytes, size_t len);
uint16_t get_transaction_id(void *bytes, size_t len);
uint16_t get_tlv_offset_by_id(uint8_t *bytes, size_t len, uint8_t tlvid);
const char *get_qmi_error_string(uint16_t result_code);
uint16_t did_qmi_op_fail(uint8_t *bytes, size_t len);
int build_qmux_header(void *output, size_t output_len, uint8_t control, uint8_t service, uint8_t instance);
int build_qmi_header(void *output, size_t output_len, uint8_t ctlid, uint16_t transaction_id, uint16_t message_id);
int build_u8_tlv(void *output, size_t output_len, size_t offset, uint8_t id, uint8_t data);
int build_u32_tlv(void *output, size_t output_len, size_t offset, uint8_t id,
                 uint32_t data);
void clear_current_transaction_id(uint8_t service);
uint16_t get_transaction_id_for_service(uint8_t service);
int add_pending_message(uint8_t service, uint8_t *buf, size_t buf_len);
void *init_internal_qmi_client();
uint8_t is_internal_qmi_client_ready();
uint16_t count_tlvs_in_message(uint8_t *bytes, size_t len);
void *start_service_initialization_thread();
#endif