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
    {0x0000, "QMI_Error: none"},
    {0x0001, "QMI_Error: malformed_msg"},
    {0x0002, "QMI_Error: no_memory"},
    {0x0003, "QMI_Error: internal"},
    {0x0004, "QMI_Error: aborted"},
    {0x0005, "QMI_Error: client_ids_exhausted"},
    {0x0006, "QMI_Error: unabortable_transaction"},
    {0x0007, "QMI_Error: invalid_client_id"},
    {0x0008, "QMI_Error: no_thresholds"},
    {0x0009, "QMI_Error: invalid_handle"},
    {0x000A, "QMI_Error: invalid_profile"},
    {0x000B, "QMI_Error: invalid_pinid"},
    {0x000C, "QMI_Error: incorrect_pin"},
    {0x000D, "QMI_Error: no_network_found"},
    {0x000E, "QMI_Error: call_failed"},
    {0x000F, "QMI_Error: out_of_call"},
    {0x0010, "QMI_Error: not_provisioned"},
    {0x0011, "QMI_Error: missing_arg"},
    {0x0013, "QMI_Error: arg_too_long"},
    {0x0016, "QMI_Error: invalid_tx_id"},
    {0x0017, "QMI_Error: device_in_use"},
    {0x0018, "QMI_Error: op_network_unsupported"},
    {0x0019, "QMI_Error: op_device_unsupported"},
    {0x001A, "QMI_Error: no_effect"},
    {0x001B, "QMI_Error: no_free_profile"},
    {0x001C, "QMI_Error: invalid_pdp_type"},
    {0x001D, "QMI_Error: invalid_tech_pref"},
    {0x001E, "QMI_Error: invalid_profile_type"},
    {0x001F, "QMI_Error: invalid_service_type"},
    {0x0020, "QMI_Error: invalid_register_action"},
    {0x0021, "QMI_Error: invalid_ps_attach_action"},
    {0x0022, "QMI_Error: authentication_failed"},
    {0x0023, "QMI_Error: pin_blocked"},
    {0x0024, "QMI_Error: pin_perm_blocked"},
    {0x0025, "QMI_Error: sim_not_initialized"},
    {0x0026, "QMI_Error: max_qos_requests_in_use"},
    {0x0027, "QMI_Error: incorrect_flow_filter"},
    {0x0028, "QMI_Error: network_qos_unaware"},
    {0x0029, "QMI_Error: invalid_id"},
    {0x0029, "QMI_Error: invalid_qos_id"},
    {0x002A, "QMI_Error: requested_num_unsupported"},
    {0x002B, "QMI_Error: interface_not_found"},
    {0x002C, "QMI_Error: flow_suspended"},
    {0x002D, "QMI_Error: invalid_data_format"},
    {0x002E, "QMI_Error: general"},
    {0x002F, "QMI_Error: unknown"},
    {0x0030, "QMI_Error: invalid_arg"},
    {0x0031, "QMI_Error: invalid_index"},
    {0x0032, "QMI_Error: no_entry"},
    {0x0033, "QMI_Error: device_storage_full"},
    {0x0034, "QMI_Error: device_not_ready"},
    {0x0035, "QMI_Error: network_not_ready"},
    {0x0036, "QMI_Error: cause_code"},
    {0x0037, "QMI_Error: message_not_sent"},
    {0x0038, "QMI_Error: message_delivery_failure"},
    {0x0039, "QMI_Error: invalid_message_id"},
    {0x003A, "QMI_Error: encoding"},
    {0x003B, "QMI_Error: authentication_lock"},
    {0x003C, "QMI_Error: invalid_transition"},
    {0x003D, "QMI_Error: not_a_mcast_iface"},
    {0x003E, "QMI_Error: max_mcast_requests_in_use"},
    {0x003F, "QMI_Error: invalid_mcast_handle"},
    {0x0040, "QMI_Error: invalid_ip_family_pref"},
    {0x0041, "QMI_Error: session_inactive"},
    {0x0042, "QMI_Error: session_invalid"},
    {0x0043, "QMI_Error: session_ownership"},
    {0x0044, "QMI_Error: insufficient_resources"},
    {0x0045, "QMI_Error: disabled"},
    {0x0046, "QMI_Error: invalid_operation"},
    {0x0047, "QMI_Error: invalid_qmi_cmd"},
    {0x0048, "QMI_Error: tpdu_type"},
    {0x0049, "QMI_Error: smsc_addr"},
    {0x004A, "QMI_Error: info_unavailable"},
    {0x004B, "QMI_Error: segment_too_long"},
    {0x004C, "QMI_Error: segment_order"},
    {0x004D, "QMI_Error: bundling_not_supported"},
    {0x004E, "QMI_Error: op_partial_failure"},
    {0x004F, "QMI_Error: policy_mismatch"},
    {0x0050, "QMI_Error: sim_file_not_found"},
    {0x0051, "QMI_Error: extended_internal"},
    {0x0052, "QMI_Error: access_denied"},
    {0x0053, "QMI_Error: hardware_restricted"},
    {0x0054, "QMI_Error: ack_not_sent"},
    {0x0055, "QMI_Error: inject_timeout"},
    {0x005A, "QMI_Error: incompatible_state"},
    {0x005B, "QMI_Error: fdn_restrict"},
    {0x005C, "QMI_Error: sups_failure_cause"},
    {0x005D, "QMI_Error: no_radio"},
    {0x005E, "QMI_Error: not_supported"},
    {0x005F, "QMI_Error: no_subscription"},
    {0x0060, "QMI_Error: card_call_control_failed"},
    {0x0061, "QMI_Error: network_aborted"},
    {0x0062, "QMI_Error: msg_blocked"},
    {0x0064, "QMI_Error: invalid_session_type"},
    {0x0065, "QMI_Error: invalid_pb_type"},
    {0x0066, "QMI_Error: no_sim"},
    {0x0067, "QMI_Error: pb_not_ready"},
    {0x0068, "QMI_Error: pin_restriction"},
    {0x0069, "QMI_Error: pin2_restriction"},
    {0x006A, "QMI_Error: puk_restriction"},
    {0x006B, "QMI_Error: puk2_restriction"},
    {0x006C, "QMI_Error: pb_access_restricted"},
    {0x006D, "QMI_Error: pb_delete_in_prog"},
    {0x006E, "QMI_Error: pb_text_too_long"},
    {0x006F, "QMI_Error: pb_number_too_long"},
    {0x0070, "QMI_Error: pb_hidden_key_restriction"},
    {0x0071, "QMI_Error: pb_not_available"},
    {0x0072, "QMI_Error: device_memory_error"},
    {0x0073, "QMI_Error: no_permission"},
    {0x0074, "QMI_Error: too_soon"},
    {0x0075, "QMI_Error: time_not_acquired"},
    {0x0076, "QMI_Error: op_in_progress"},
    {0x101, "QMI_Error: eperm"},
    {0x102, "QMI_Error: enoent"},
    {0x103, "QMI_Error: esrch"},
    {0x104, "QMI_Error: eintr"},
    {0x105, "QMI_Error: eio"},
    {0x106, "QMI_Error: enxio"},
    {0x107, "QMI_Error: e2big"},
    {0x108, "QMI_Error: enoexec"},
    {0x109, "QMI_Error: ebadf"},
    {0x10A, "QMI_Error: echild"},
    {0x10B, "QMI_Error: eagain"},
    {0x10C, "QMI_Error: enomem"},
    {0x10D, "QMI_Error: eacces"},
    {0x10E, "QMI_Error: efault"},
    {0x10F, "QMI_Error: enotblk"},
    {0x110, "QMI_Error: ebusy"},
    {0x111, "QMI_Error: eexist"},
    {0x112, "QMI_Error: exdev"},
    {0x113, "QMI_Error: enodev"},
    {0x114, "QMI_Error: enotdir"},
    {0x115, "QMI_Error: eisdir"},
    {0x116, "QMI_Error: einval"},
    {0x117, "QMI_Error: enfile"},
    {0x118, "QMI_Error: emfile"},
    {0x119, "QMI_Error: enotty"},
    {0x11A, "QMI_Error: etxtbsy"},
    {0x11B, "QMI_Error: efbig"},
    {0x11C, "QMI_Error: enospc"},
    {0x11D, "QMI_Error: espipe"},
    {0x11E, "QMI_Error: erofs"},
    {0x11F, "QMI_Error: emlink"},
    {0x120, "QMI_Error: epipe"},
    {0x121, "QMI_Error: edom"},
    {0x122, "QMI_Error: erange"},
    {0x123, "QMI_Error: edeadlk"},
    {0x124, "QMI_Error: enametoolong"},
    {0x125, "QMI_Error: enolck"},
    {0x126, "QMI_Error: enosys"},
    {0x127, "QMI_Error: enotempty"},
    {0x128, "QMI_Error: eloop"},
    {0x10B, "QMI_Error: ewouldblock"},
    {0x12A, "QMI_Error: enomsg"},
    {0x12B, "QMI_Error: eidrm"},
    {0x12C, "QMI_Error: echrng"},
    {0x12D, "QMI_Error: el2nsync"},
    {0x12E, "QMI_Error: el3hlt"},
    {0x12F, "QMI_Error: el3rst"},
    {0x130, "QMI_Error: elnrng"},
    {0x131, "QMI_Error: eunatch"},
    {0x132, "QMI_Error: enocsi"},
    {0x133, "QMI_Error: el2hlt"},
    {0x134, "QMI_Error: ebade"},
    {0x135, "QMI_Error: ebadr"},
    {0x136, "QMI_Error: exfull"},
    {0x137, "QMI_Error: enoano"},
    {0x138, "QMI_Error: ebadrqc"},
    {0x139, "QMI_Error: ebadslt"},
    {0x123, "QMI_Error: edeadlock"},
    {0x13B, "QMI_Error: ebfont"},
    {0x13C, "QMI_Error: enostr"},
    {0x13D, "QMI_Error: enodata"},
    {0x13E, "QMI_Error: etime"},
    {0x13F, "QMI_Error: enosr"},
    {0x140, "QMI_Error: enonet"},
    {0x141, "QMI_Error: enopkg"},
    {0x142, "QMI_Error: eremote"},
    {0x143, "QMI_Error: enolink"},
    {0x144, "QMI_Error: eadv"},
    {0x145, "QMI_Error: esrmnt"},
    {0x146, "QMI_Error: ecomm"},
    {0x147, "QMI_Error: eproto"},
    {0x148, "QMI_Error: emultihop"},
    {0x149, "QMI_Error: edotdot"},
    {0x14A, "QMI_Error: ebadmsg"},
    {0x14B, "QMI_Error: eoverflow"},
    {0x14C, "QMI_Error: enotuniq"},
    {0x14D, "QMI_Error: ebadfd"},
    {0x14E, "QMI_Error: eremchg"},
    {0x14F, "QMI_Error: elibacc"},
    {0x150, "QMI_Error: elibbad"},
    {0x151, "QMI_Error: elibscn"},
    {0x152, "QMI_Error: elibmax"},
    {0x153, "QMI_Error: elibexec"},
    {0x154, "QMI_Error: eilseq"},
    {0x155, "QMI_Error: erestart"},
    {0x156, "QMI_Error: estrpipe"},
    {0x157, "QMI_Error: eusers"},
    {0x158, "QMI_Error: enotsock"},
    {0x159, "QMI_Error: edestaddrreq"},
    {0x15A, "QMI_Error: emsgsize"},
    {0x15B, "QMI_Error: eprototype"},
    {0x15C, "QMI_Error: enoprotoopt"},
    {0x15D, "QMI_Error: eprotonosupport"},
    {0x15E, "QMI_Error: esocktnosupport"},
    {0x15F, "QMI_Error: eopnotsupp"},
    {0x160, "QMI_Error: epfnosupport"},
    {0x161, "QMI_Error: eafnosupport"},
    {0x162, "QMI_Error: eaddrinuse"},
    {0x163, "QMI_Error: eaddrnotavail"},
    {0x164, "QMI_Error: enetdown"},
    {0x165, "QMI_Error: enetunreach"},
    {0x166, "QMI_Error: enetreset"},
    {0x167, "QMI_Error: econnaborted"},
    {0x168, "QMI_Error: econnreset"},
    {0x169, "QMI_Error: enobufs"},
    {0x16A, "QMI_Error: eisconn"},
    {0x16B, "QMI_Error: enotconn"},
    {0x16C, "QMI_Error: eshutdown"},
    {0x16D, "QMI_Error: etoomanyrefs"},
    {0x16E, "QMI_Error: etimedout"},
    {0x16F, "QMI_Error: econnrefused"},
    {0x170, "QMI_Error: ehostdown"},
    {0x171, "QMI_Error: ehostunreach"},
    {0x172, "QMI_Error: ealready"},
    {0x173, "QMI_Error: einprogress"},
    {0x174, "QMI_Error: estale"},
    {0x175, "QMI_Error: euclean"},
    {0x176, "QMI_Error: enotnam"},
    {0x177, "QMI_Error: enavail"},
    {0x178, "QMI_Error: eisnam"},
    {0x179, "QMI_Error: eremoteio"},
    {0x17A, "QMI_Error: edquot"},
    {0x17B, "QMI_Error: enomedium"},
    {0x17C, "QMI_Error: emediumtype"},
    {0x17D, "QMI_Error: ecanceled"},
    {0x17E, "QMI_Error: enokey"},
    {0x17F, "QMI_Error: ekeyexpired"},
    {0x180, "QMI_Error: ekeyrevoked"},
    {0x181, "QMI_Error: ekeyrejected"},
    {0x182, "QMI_Error: eownerdead"},
    {0x183, "QMI_Error: enotrecoverable"},
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

uint8_t get_qmux_service_id(void *bytes, size_t len);
uint8_t get_qmux_instance_id(void *bytes, size_t len);
uint16_t get_control_message_id(void *bytes, size_t len);
uint16_t get_qmi_message_id(void *bytes, size_t len);
uint16_t get_qmi_transaction_id(void *bytes, size_t len);
uint16_t get_transaction_id(void *bytes, size_t len);
uint16_t get_tlv_offset_by_id(uint8_t *bytes, size_t len, uint8_t tlvid);
uint16_t did_qmi_op_fail(uint8_t *bytes, size_t len);
int build_qmux_header(void *output, size_t output_len, uint8_t control, uint8_t service, uint8_t instance);
int build_qmi_header(void *output, size_t output_len, uint8_t ctlid, uint16_t transaction_id, uint16_t message_id);
#endif