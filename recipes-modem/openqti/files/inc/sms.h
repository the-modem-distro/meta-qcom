/* SPDX-License-Identifier: MIT */

#ifndef _SMS_H
#define _SMS_H
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_MESSAGE_SIZE 160
#define QUEUE_SIZE 100
#define MAX_PHONE_NUMBER_SIZE 20

/* OpenQTI's way of knowing if it
  needs to trigger an internal or
  external indication message */
enum {
  MSG_NONE = -1,
  MSG_EXTERNAL = 0,
  MSG_INTERNAL = 1,
};

/* QMI message IDs for SMS/MMS */
enum {
  WMS_RESET = 0x000,
  WMS_EVENT_REPORT = 0x0001,
  WMS_GET_SUPPORTED_MESSAGES = 0x001e,
  WMS_GET_SUPPORTED_FIELDS = 0x001f,
  WMS_RAW_SEND = 0x0020,
  WMS_RAW_WRITE = 0x0021,
  WMS_READ_MESSAGE = 0x0022,
  WMS_DELETE = 0x0024,
  WMS_GET_MSG_PROTOCOL = 0x0030,
};

/* For GSM7 message decoding */
enum {
  BITMASK_7BITS = 0x7F,
  BITMASK_8BITS = 0xFF,
  BITMASK_HIGH_4BITS = 0xF0,
  TYPE_OF_ADDRESS_INTERNATIONAL_PHONE = 0x91,
  TYPE_OF_ADDRESS_NATIONAL_SUBSCRIBER = 0xC8,
};

enum {
  TLV_QMI_RESULT = 0x02,
  TLV_MESSAGE_TYPE = 0x10,
  TLV_MESSAGE_MODE = 0x12,
  TLV_SMS_OVER_IMS = 0x16,
};

/* Define Non-Printable Characters as a question mark */
#define NPC7 63
#define NPC8 '?'
/*
https://github.com/legatoproject/legato-af/blob/master/components/modemServices/modemDaemon/smsPdu.c
*/
/****************************************************************************
 * This lookup table converts from ISO-8859-1 8-bit ASCII to the
 * 7 bit "default alphabet" as defined in ETSI GSM 03.38
 *
 * ISO-characters that don't have any corresponding character in the
 * 7-bit alphabet is replaced with the NPC7-character.  If there's
 * a close match between the ISO-char and a 7-bit character (for example
 * the letter i with a circumflex and the plain i-character) a substitution
 * is done.
 *
 * There are some character (for example the square brace "]") that must
 * be converted into a 2 byte 7-bit sequence.  These characters are
 * marked in the table by having 128 added to its value.
 ****************************************************************************/
static const uint8_t Ascii8to7[] = {
    NPC7,         /*     0      null [NUL]                              */
    NPC7,         /*     1      start of heading [SOH]                  */
    NPC7,         /*     2      start of text [STX]                     */
    NPC7,         /*     3      end of text [ETX]                       */
    NPC7,         /*     4      end of transmission [EOT]               */
    NPC7,         /*     5      enquiry [ENQ]                           */
    NPC7,         /*     6      acknowledge [ACK]                       */
    NPC7,         /*     7      bell [BEL]                              */
    NPC7,         /*     8      backspace [BS]                          */
    NPC7,         /*     9      horizontal tab [HT]                     */
    10,           /*    10      line feed [LF]                          */
    NPC7,         /*    11      vertical tab [VT]                       */
    10 + 128,     /*    12      form feed [FF]                          */
    13,           /*    13      carriage return [CR]                    */
    NPC7,         /*    14      shift out [SO]                          */
    NPC7,         /*    15      shift in [SI]                           */
    NPC7,         /*    16      data link escape [DLE]                  */
    NPC7,         /*    17      device control 1 [DC1]                  */
    NPC7,         /*    18      device control 2 [DC2]                  */
    NPC7,         /*    19      device control 3 [DC3]                  */
    NPC7,         /*    20      device control 4 [DC4]                  */
    NPC7,         /*    21      negative acknowledge [NAK]              */
    NPC7,         /*    22      synchronous idle [SYN]                  */
    NPC7,         /*    23      end of trans. block [ETB]               */
    NPC7,         /*    24      cancel [CAN]                            */
    NPC7,         /*    25      end of medium [EM]                      */
    NPC7,         /*    26      substitute [SUB]                        */
    NPC7,         /*    27      escape [ESC]                            */
    NPC7,         /*    28      file separator [FS]                     */
    NPC7,         /*    29      group separator [GS]                    */
    NPC7,         /*    30      record separator [RS]                   */
    NPC7,         /*    31      unit separator [US]                     */
    32,           /*    32      space                                   */
    33,           /*    33    ! exclamation mark                        */
    34,           /*    34    " double quotation mark                   */
    35,           /*    35    # number sign                             */
    2,            /*    36    $ dollar sign                             */
    37,           /*    37    % percent sign                            */
    38,           /*    38    & ampersand                               */
    39,           /*    39    ' apostrophe                              */
    40,           /*    40    ( left parenthesis                        */
    41,           /*    41    ) right parenthesis                       */
    42,           /*    42    * asterisk                                */
    43,           /*    43    + plus sign                               */
    44,           /*    44    , comma                                   */
    45,           /*    45    - hyphen                                  */
    46,           /*    46    . period                                  */
    47,           /*    47    / slash,                                  */
    48,           /*    48    0 digit 0                                 */
    49,           /*    49    1 digit 1                                 */
    50,           /*    50    2 digit 2                                 */
    51,           /*    51    3 digit 3                                 */
    52,           /*    52    4 digit 4                                 */
    53,           /*    53    5 digit 5                                 */
    54,           /*    54    6 digit 6                                 */
    55,           /*    55    7 digit 7                                 */
    56,           /*    56    8 digit 8                                 */
    57,           /*    57    9 digit 9                                 */
    58,           /*    58    : colon                                   */
    59,           /*    59    ; semicolon                               */
    60,           /*    60    < less-than sign                          */
    61,           /*    61    = equal sign                              */
    62,           /*    62    > greater-than sign                       */
    63,           /*    63    ? question mark                           */
    0,            /*    64    @ commercial at sign                      */
    65,           /*    65    A uppercase A                             */
    66,           /*    66    B uppercase B                             */
    67,           /*    67    C uppercase C                             */
    68,           /*    68    D uppercase D                             */
    69,           /*    69    E uppercase E                             */
    70,           /*    70    F uppercase F                             */
    71,           /*    71    G uppercase G                             */
    72,           /*    72    H uppercase H                             */
    73,           /*    73    I uppercase I                             */
    74,           /*    74    J uppercase J                             */
    75,           /*    75    K uppercase K                             */
    76,           /*    76    L uppercase L                             */
    77,           /*    77    M uppercase M                             */
    78,           /*    78    N uppercase N                             */
    79,           /*    79    O uppercase O                             */
    80,           /*    80    P uppercase P                             */
    81,           /*    81    Q uppercase Q                             */
    82,           /*    82    R uppercase R                             */
    83,           /*    83    S uppercase S                             */
    84,           /*    84    T uppercase T                             */
    85,           /*    85    U uppercase U                             */
    86,           /*    86    V uppercase V                             */
    87,           /*    87    W uppercase W                             */
    88,           /*    88    X uppercase X                             */
    89,           /*    89    Y uppercase Y                             */
    90,           /*    90    Z uppercase Z                             */
    60 + 128,     /*    91    [ left square bracket                     */
    47 + 128,     /*    92    \ backslash                               */
    62 + 128,     /*    93    ] right square bracket                    */
    20 + 128,     /*    94    ^ circumflex accent                       */
    17,           /*    95    _ underscore                              */
    (uint8_t)-39, /*  96    ` back apostrophe                         */
    97,           /*    97    a lowercase a                             */
    98,           /*    98    b lowercase b                             */
    99,           /*    99    c lowercase c                             */
    100,          /*   100    d lowercase d                             */
    101,          /*   101    e lowercase e                             */
    102,          /*   102    f lowercase f                             */
    103,          /*   103    g lowercase g                             */
    104,          /*   104    h lowercase h                             */
    105,          /*   105    i lowercase i                             */
    106,          /*   106    j lowercase j                             */
    107,          /*   107    k lowercase k                             */
    108,          /*   108    l lowercase l                             */
    109,          /*   109    m lowercase m                             */
    110,          /*   110    n lowercase n                             */
    111,          /*   111    o lowercase o                             */
    112,          /*   112    p lowercase p                             */
    113,          /*   113    q lowercase q                             */
    114,          /*   114    r lowercase r                             */
    115,          /*   115    s lowercase s                             */
    116,          /*   116    t lowercase t                             */
    117,          /*   117    u lowercase u                             */
    118,          /*   118    v lowercase v                             */
    119,          /*   119    w lowercase w                             */
    120,          /*   120    x lowercase x                             */
    121,          /*   121    y lowercase y                             */
    122,          /*   122    z lowercase z                             */
    40 + 128,     /*   123    { left brace                              */
    64 + 128,     /*   124    | vertical bar                            */
    41 + 128,     /*   125    } right brace                             */
    61 + 128,     /*   126    ~ tilde accent                            */
    NPC7,         /*   127      delete [DEL]                            */
    NPC7,         /*   128                                              */
    NPC7,         /*   129                                              */
    39,           /*   130      low left rising single quote            */
    102,          /*   131      lowercase italic f                      */
    34,           /*   132      low left rising double quote            */
    NPC7,         /*   133      low horizontal ellipsis                 */
    NPC7,         /*   134      dagger mark                             */
    NPC7,         /*   135      double dagger mark                      */
    NPC7,         /*   136      letter modifying circumflex             */
    NPC7,         /*   137      per thousand (mille) sign               */
    83,           /*   138      uppercase S caron or hacek              */
    39,           /*   139      left single angle quote mark            */
    214,          /*   140      uppercase OE ligature                   */
    NPC7,         /*   141                                              */
    NPC7,         /*   142                                              */
    NPC7,         /*   143                                              */
    NPC7,         /*   144                                              */
    39,           /*   145      left single quotation mark              */
    39,           /*   146      right single quote mark                 */
    34,           /*   147      left double quotation mark              */
    34,           /*   148      right double quote mark                 */
    42,           /*   149      round filled bullet                     */
    45,           /*   150      en dash                                 */
    45,           /*   151      em dash                                 */
    39,           /*   152      small spacing tilde accent              */
    NPC7,         /*   153      trademark sign                          */
    115,          /*   154      lowercase s caron or hacek              */
    39,           /*   155      right single angle quote mark           */
    111,          /*   156      lowercase oe ligature                   */
    NPC7,         /*   157                                              */
    NPC7,         /*   158                                              */
    89,           /*   159      uppercase Y dieresis or umlaut          */
    32,           /*   160      non-breaking space                      */
    64,           /*   161    ¡ inverted exclamation mark               */
    99,           /*   162    ¢ cent sign                               */
    1,            /*   163    £ pound sterling sign                     */
    36,           /*   164    € general currency sign                   */
    3,            /*   165    ¥ yen sign                                */
    33,           /*   166    Š broken vertical bar                     */
    95,           /*   167    § section sign                            */
    34,           /*   168    š spacing dieresis or umlaut              */
    NPC7,         /*   169    © copyright sign                          */
    NPC7,         /*   170    ª feminine ordinal indicator              */
    60,           /*   171    « left (double) angle quote               */
    NPC7,         /*   172    ¬ logical not sign                        */
    45,           /*   173    ­ soft hyphen                             */
    NPC7,         /*   174    ® registered trademark sign               */
    NPC7,         /*   175    ¯ spacing macron (long) accent            */
    NPC7,         /*   176    ° degree sign                             */
    NPC7,         /*   177    ± plus-or-minus sign                      */
    50,           /*   178    ² superscript 2                           */
    51,           /*   179    ³ superscript 3                           */
    39,           /*   180    Ž spacing acute accent                    */
    117,          /*   181    µ micro sign                              */
    NPC7,         /*   182    ¶ paragraph sign, pilcrow sign            */
    NPC7,         /*   183    · middle dot, centered dot                */
    NPC7,         /*   184    ž spacing cedilla                         */
    49,           /*   185    ¹ superscript 1                           */
    NPC7,         /*   186    º masculine ordinal indicator             */
    62,           /*   187    » right (double) angle quote (guillemet)  */
    NPC7,         /*   188    Œ fraction 1/4                            */
    NPC7,         /*   189    œ fraction 1/2                            */
    NPC7,         /*   190    Ÿ fraction 3/4                            */
    96,           /*   191    ¿ inverted question mark                  */
    65,           /*   192    À uppercase A grave                       */
    65,           /*   193    Á uppercase A acute                       */
    65,           /*   194    Â uppercase A circumflex                  */
    65,           /*   195    Ã uppercase A tilde                       */
    91,           /*   196    Ä uppercase A dieresis or umlaut          */
    14,           /*   197    Å uppercase A ring                        */
    28,           /*   198    Æ uppercase AE ligature                   */
    9,            /*   199    Ç uppercase C cedilla                     */
    31,           /*   200    È uppercase E grave                       */
    31,           /*   201    É uppercase E acute                       */
    31,           /*   202    Ê uppercase E circumflex                  */
    31,           /*   203    Ë uppercase E dieresis or umlaut          */
    73,           /*   204    Ì uppercase I grave                       */
    73,           /*   205    Í uppercase I acute                       */
    73,           /*   206    Î uppercase I circumflex                  */
    73,           /*   207    Ï uppercase I dieresis or umlaut          */
    68,           /*   208    Ð uppercase ETH                           */
    93,           /*   209    Ñ uppercase N tilde                       */
    79,           /*   210    Ò uppercase O grave                       */
    79,           /*   211    Ó uppercase O acute                       */
    79,           /*   212    Ô uppercase O circumflex                  */
    79,           /*   213    Õ uppercase O tilde                       */
    92,           /*   214    Ö uppercase O dieresis or umlaut          */
    42,           /*   215    × multiplication sign                     */
    11,           /*   216    Ø uppercase O slash                       */
    85,           /*   217    Ù uppercase U grave                       */
    85,           /*   218    Ú uppercase U acute                       */
    85,           /*   219    Û uppercase U circumflex                  */
    94,           /*   220    Ü uppercase U dieresis or umlaut          */
    89,           /*   221    Ý uppercase Y acute                       */
    NPC7,         /*   222    Þ uppercase THORN                         */
    30,           /*   223    ß lowercase sharp s, sz ligature          */
    127,          /*   224    à lowercase a grave                       */
    97,           /*   225    á lowercase a acute                       */
    97,           /*   226    â lowercase a circumflex                  */
    97,           /*   227    ã lowercase a tilde                       */
    123,          /*   228    ä lowercase a dieresis or umlaut          */
    15,           /*   229    å lowercase a ring                        */
    29,           /*   230    æ lowercase ae ligature                   */
    9,            /*   231    ç lowercase c cedilla                     */
    4,            /*   232    è lowercase e grave                       */
    5,            /*   233    é lowercase e acute                       */
    101,          /*   234    ê lowercase e circumflex                  */
    101,          /*   235    ë lowercase e dieresis or umlaut          */
    7,            /*   236    ì lowercase i grave                       */
    7,            /*   237    í lowercase i acute                       */
    105,          /*   238    î lowercase i circumflex                  */
    105,          /*   239    ï lowercase i dieresis or umlaut          */
    NPC7,         /*   240    ð lowercase eth                           */
    125,          /*   241    ñ lowercase n tilde                       */
    8,            /*   242    ò lowercase o grave                       */
    111,          /*   243    ó lowercase o acute                       */
    111,          /*   244    ô lowercase o circumflex                  */
    111,          /*   245    õ lowercase o tilde                       */
    24,           /*   246    ö lowercase o dieresis or umlaut          */
    47,           /*   247    ÷ division sign                           */
    12,           /*   248    ø lowercase o slash                       */
    6,            /*   249    ù lowercase u grave                       */
    117,          /*   250    ú lowercase u acute                       */
    117,          /*   251    û lowercase u circumflex                  */
    126,          /*   252    ü lowercase u dieresis or umlaut          */
    121,          /*   253    ý lowercase y acute                       */
    NPC7,         /*   254    þ lowercase thorn                         */
    121           /*   255    ÿ lowercase y dieresis or umlaut          */
};

/*
 *  <-- qmi.h [struct qmux_packet]
 *  <-- qmi.h [struct qmi_packet]
 *  <-- qmi.h [struct qmi_generic_result_ind]
 */

struct sms_storage_type { // 8byte
  /* Message storage */
  uint8_t tlv_message_type;   // 0x10
  uint16_t tlv_msg_type_size; //  5 bytes
  uint8_t storage_type;       // 00 -> UIM, 01 -> NV
  uint32_t message_id;        // Index! +CMTI: "ME", [MESSAGE_ID]
} __attribute__((packed));

struct sms_message_mode {
  /* Message mode */
  uint8_t tlv_message_mode; // 0x12
  uint16_t tlv_mode_size;   // 0x01
  uint8_t message_mode;     // 0x01 GSM
} __attribute__((packed));

struct sms_over_ims {
  /* SMS on IMS? */
  uint8_t tlv_sms_on_ims;       // 0x16
  uint16_t tlv_sms_on_ims_size; // 0x01
  uint8_t is_sms_sent_over_ims; // 0x00 || 0x01 [no | yes]
} __attribute__((packed));

struct wms_message_indication_packet { // 0x0001
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Storage - where is the message? */
  struct sms_storage_type storage;
  /* Mode - type of message? */
  struct sms_message_mode mode;
  /* SMS over IMS? Yes or no */
  struct sms_over_ims ims;
} __attribute__((packed));

struct wms_message_delete_packet { // 0x0024
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Did we delete it from our (false) storage? */
  struct qmi_generic_result_ind indication;
} __attribute__((packed));

struct wms_message_settings {
  uint8_t message_tlv; // 0x01
  uint16_t size;       // REMAINING SIZE OF PKT (!!)
  //??   uint8_t message_tag; // 0x00 read, 1 unread, 2 sent, 3, unsent, 4??
  uint8_t message_storage; // 0x00 || 0x01 (sim/mem?)
  uint8_t format;          // always 0x06
} __attribute__((packed));

struct wms_raw_message_header {
  uint8_t message_tlv; // 0x01 RAW MSG
  uint16_t size;       // REMAINING SIZE OF PKT (!!)
  uint8_t tlv_version; // 0x01
} __attribute__((packed));

struct generic_tlv_onebyte { // 4byte
  uint8_t id;                // 0x01 RAW MSG
  uint16_t size;             // REMAINING SIZE OF PKT (!!)
  uint8_t data;              // 0x01
} __attribute__((packed));

struct wms_message_target_data {
  uint16_t message_size; // what remains in the packet

} __attribute__((packed));

struct wms_phone_number {
  uint8_t phone_number_size; // SMSC counts in gsm7, target in ascii (??!!?)
  uint8_t is_international_number; // 0x91 => +
  uint8_t number[MAX_PHONE_NUMBER_SIZE];
} __attribute__((packed));

struct wms_hardcoded_phone_number {
  uint8_t phone_number_size; // SMSC counts in gsm7, target in ascii (??!!?)
  uint8_t is_international_number; // 0x91 => +
  uint8_t number[6];
} __attribute__((packed));

struct wms_datetime {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t timezone; // 0x40 ??
} __attribute__((packed));

struct wms_message_contents {
  uint8_t content_sz; // Size *AFTER* conversion
  uint8_t contents[MAX_MESSAGE_SIZE];
} __attribute__((packed));

struct wms_user_data {
  uint8_t tlv; // 0x06
  uint16_t user_data_size;
  struct wms_hardcoded_phone_number smsc;  // source smsc
  uint8_t unknown;                         // 0x04 (!)
  struct wms_hardcoded_phone_number phone; // source numb
  uint8_t tp_pid;                          // Not sure at all, 0x00
  uint8_t tp_dcs;                          // Not sure at all, 0x00
  struct wms_datetime date;
  /* Actual data for the message */
  struct wms_message_contents contents;
} __attribute__((packed));

struct wms_build_message {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Did it succeed? */
  struct qmi_generic_result_ind indication;
  /* This message tag and format */
  struct wms_raw_message_header header;
  /* Size of smsc + phone + date + tp* + contents */
  struct wms_user_data data;
} __attribute__((packed));

struct wms_request_message {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Message tag */
  struct generic_tlv_onebyte message_tag;
  /* Request data */
  struct sms_storage_type storage;
} __attribute__((packed));

struct wms_request_message_ofono {
  /* QMUX header */
  struct qmux_packet qmuxpkt;
  /* QMI header */
  struct qmi_packet qmipkt;
  /* Request data */
  struct sms_storage_type storage;
  /* Message tag */
  struct generic_tlv_onebyte message_tag;
} __attribute__((packed));

/* Messages outgoing from the
 * host to the modem. This needs
 * rebuilding too */

struct sms_outgoing_header {
  uint8_t unknown; // 0x01
  uint16_t size1;
} __attribute__((packed));

struct smsc_data {
  uint8_t sz;
  uint8_t smsc_number[7];
} __attribute__((packed));

struct sms_caller_data {
  uint8_t unknown;
  uint8_t sz; // Size of phone number *AFTER* converting to 8bit
  uint8_t phone_number[7];
} __attribute__((packed));

struct sms_content {
  uint8_t content_sz; // Size *AFTER* conversion
  uint8_t contents[MAX_MESSAGE_SIZE];
} __attribute__((packed));

struct outgoing_sms_packet {
  struct qmux_packet qmuxpkt;
  struct qmi_packet qmipkt;
  struct sms_outgoing_header header; // RAW MESSAGE INDICATION
  struct sms_outgoing_header
      header_tlv2; // TYPE OF MESSAGE TO BE SENT, 0x06 -> 3GPP

  uint8_t sca_length; // 0x00 Indicates if it has a SMSC set. It shouldn't, we
                      // ignore it.
  uint8_t pdu_type;   // 0xX1 == SUBMIT,  >=0x11 includes validity period
  struct sms_caller_data
      target;     // 7bit gsm encoded htole, 0xf on last item if padding needed
  uint8_t tp_pid; // 0x00
  uint8_t tp_dcs; // 0x00
  uint8_t validity_period;     // 0x21 || 0xa7
  struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

struct sms_received_ack {
  struct qmux_packet qmuxpkt;
  struct qmi_packet qmipkt;
  struct qmi_generic_result_ind indication;

  uint8_t message_tlv_id;
  uint16_t message_id_len;
  uint16_t message_id;
} __attribute__((packed));

struct outgoing_no_validity_period_sms_packet {
  struct qmux_packet qmuxpkt;
  struct qmi_packet qmipkt;
  struct sms_outgoing_header header;
  struct sms_outgoing_header header_tlv2;

  uint8_t sca_length; // 0x00 Indicates if it has a SMSC set. It shouldn't, we
                      // ignore it.
  uint8_t pdu_type;   // 0xX1 == SUBMIT, >=0x11 includes validity period
  struct sms_caller_data
      target;    // 7bit gsm encoded htole, 0xf on last item if padding needed
  uint16_t unk4; // 0x00 0x00
  struct sms_content contents; // 7bit gsm encoded data
} __attribute__((packed));

/* Functions */
void reset_sms_runtime();
void set_notif_pending(bool en);
void set_pending_notification_source(uint8_t source);
uint8_t get_notification_source();
bool is_message_pending();
uint8_t generate_message_notification(int fd, uint32_t message_id);
uint8_t ack_message_notification(int fd, uint8_t pending_message_num);
uint8_t inject_message(uint8_t message_id);
uint8_t do_inject_notification(int fd);

uint8_t intercept_and_parse(void *bytes, size_t len, int hostfd, int adspfd);

int process_message_queue(int fd);
void add_sms_to_queue(uint8_t *message, size_t len);
void notify_wms_event(uint8_t *bytes, size_t len, int fd);
int check_wms_message(uint8_t source, void *bytes, size_t len, int adspfd,
                      int usbfd);
int check_wms_indication_message(void *bytes, size_t len, int adspfd,
                                 int usbfd);
int check_cb_message(void *bytes, size_t len, int adspfd, int usbfd);
#endif