/* SPDX-License-Identifier: MIT */
#ifndef _ATFWD_H_
#define _ATFWD_H_
#include <stdbool.h>
#include <stdint.h>

static const struct {
  unsigned int command_id;
  const char *cmd;
} at_commands[] = {
    {0, "+QNAND"},
    {1, "+QPRTPARA"},
    {2, "+QWSERVER"},
    {3, "+QWTOCLIEN"},
    {4, "+QWTOCLI"},
    {5, "+QWPARAM"},
    {6, "+QDATAFWD"},
    {7, "+QFTCMD"},
    {8, "+QBTPWR"},
    {9, "+QBTLEADDR"},
    {10, "+QBTNAME"},
    {11, "+QBTGATREG"},
    {12, "+QBTGATSS"},
    {13, "+QBTGATSC"},
    {14, "+QBTGATSD"},
    {15, "+QBTGATSIND"},
    {16, "+QBTGATSNOD"},
    {17, "+QBTGATRRSP"},
    {18, "+QBTGATWRSP"},
    {19, "+QBTGATSENLE"},
    {20, "+QBTGATADV"},
    {21, "+QBTGATDISC"},
    {22, "+QBTGATPER"},
    {23, "+QBTGATDBALC"},
    {24, "+QBTGATDBDEALC"},
    {25, "+QBTGATSA"},
    {26, "+QBTGATDA"},
    {27, "+QFCT"},
    {28, "+QFCTTX"},
    {29, "+QFCTRX"},
    {30, "+QBTSPPACT"},
    {31, "+QBTSPPDIC"},
    {32, "+QBTSPPWRS"},
    {33, "+QBTSCAN"},
    {34, "+QBTAVACT"},
    {35, "+QBTAVREG"},
    {36, "+QBTAVCON"},
    {37, "+QBTHFGCON"},
    {38, "+QIIC"},
    {39, "+QAUDLOOP"},
    {40, "+QDAI"},
    {41, "+QSIDET"},
    {42, "+QAUDMOD"},
    {43, "+QEEC"},
    {44, "+QMIC"},
    {45, "+QRXGAIN"},
    {46, "+QAUDRD"},
    {47, "+QAUDPLAY"},
    {48, "+QAUDSTOP"},
    {49, "+QPSND"},
    {50, "+QTTS"},
    {51, "+QTTSETUP"},
    {52, "+QLTONE"},
    {53, "+QLDTMF"},
    {54, "+QAUDCFG"},
    {55, "+QTONEDET"},
    {56, "+QWTTS"},
    {57, "+QPCMV"},
    {58, "+QTXIIR"},
    {59, "+QRXIIR"},
    {60, "+QPOWD"},
    {61, "+QSCLK"},
    {62, "+QCFG"},
    {63, "+QADBKEY"},
    {64, "+QADC"},
    {65, "+QADCTEMP"},
    {66, "+QGPSCFG"},
    {67, "+QODM"},
    {68, "+QFUMO"},
    {69, "+QFUMOCFG"},
    {70, "+QPRINT"},
    {71, "+QSDMOUNT"},
    {72, "+QFDUMMY"},
    {73, "+QPSM"},
    {74, "+QPSMCFG"},
    {75, "+QLINUXCPU"},
    {76, "+QVERSION"},
    {77, "+QSUBSYSVER"},
    {78, "+QTEMPDBG"},
    {79, "+QTEMP"},
    {80, "+QTEMPDBGLVL"},
    {81, "+QDIAGPORT"},
    {82, "+QLPMCFG"},
    {83, "+QSGMIICFG"},
    {84, "+QWWAN"},
    {85, "+QLWWANUP"},
    {86, "+QLWWANDOWN"},
    {87, "+QLWWANSTATUS"},
    {88, "+QLWWANURCCFG"},
    {89, "+QLWWANCID"},
    {90, "+QLPING"},
    {91, "+QWIFI"},
    {92, "+QWSSID"},
    {93, "+QWSSIDHEX"},
    {94, "+QWAUTH"},
    {95, "+QWMOCH"},
    {96, "+QWISO"},
    {97, "+QWBCAST"},
    {98, "+QWCLICNT"},
    {99, "+QWCLIP"},
    {100, "+QWCLILST"},
    {101, "+QWSTAINFO"},
    {102, "+QWCLIRM"},
    {103, "+QWSETMAC"},
    {104, "+QWRSTD"},
    {105, "+QWIFICFG"},
    {106, "+QAPRDYIND"},
    {107, "+QFOTADL"},
    {108, "+CFUN"},
    {109, "+CMUX"},
    {110, "+IPR"},
    // New commands start here
    {111, "+PWRDN"},
    {112, "+ADBON"},
    {113, "+ADBOFF"},
    {114, "+RESETUSB"},
    {115, "+REBOOT_REC"},
    {116, "+IS_CUSTOM"},
    {117, "+EN_PCM16K"},
    {118, "+EN_PCM48K"},
    {119, "+EN_PCM8K"},
    {120, "+EN_USBAUD"},
    {121, "+DIS_USBAUD"},
    {122, "+CMUT"},
    {123, "+REBOOTDO"},
    {124, "+EN_CAT"},
    {125, "+DIS_CAT"},
    {126, "+GETSWREV"},
    {127, "+QFASTBOOT"},
    {128, "+GETFWBRANCH"},
    {129, "+QGMR"},
    {130, "+DMESG"},
    {131, "+OQLOG"},
};


#define AT_REG_REQ 0x0020
#define AT_CMD_RES 0x0022
/*
VAR3 renamed to command_length, as it's always the length of the string being
passed +1 but removing the "+" sign ($QCPWRDN...) VAR2: When string is 4
chars, it's always 0x07, When string is 8 chars, it's always 0x0a When string
is 3 chars, it's always 0x06 VAR1 is always 3 more than VAR2
*/

struct atcmd_reg_request {
  // QMI Header
  uint8_t ctlid;           // 0x00
  uint16_t transaction_id; // incremental counter for each request
  uint16_t msgid;          // 0x20 0x00
  uint16_t packet_size;    // Sizeof the entire packet

  // the request packet itself
  // Obviously unfinished :)
  uint8_t dummy1;         // always 0x00 0x01
  uint8_t var1;           // 0x0a - 0x0f || 0x11 - 0x13
  uint16_t dummy2;        // always 0x00 0x01
  uint8_t var2;           // 0x07 - 0x0e
  uint16_t dummy3;        // always 0x00 0x01
  uint8_t command_length; // var3;
} __attribute__((packed));

#define MAX_REPLY_SZ 4096
struct qmi_packet {
  uint8_t ctlid;           // 0x00 Control message
  uint16_t transaction_id; // QMI Transaction ID
  uint16_t msgid;          // For AT IF, 0x0022 is a reply
  uint16_t length;         // QMI Packet size
} __attribute__((packed));


struct at_command_meta {
  uint8_t client_handle; // 0x01
  uint16_t at_pkt_len; // AT packet size
} __attribute__ ((packed));

struct at_command_respnse {
  struct qmi_packet qmipkt;
  struct at_command_meta meta;
  uint32_t handle; // 0x0b 0x00 0x00 0x00 
  uint8_t result; // 1 OK 2 OTHER, 0 ERR
  uint8_t response; // 3 == Complete response in one packet
  uint16_t replysz;
  char reply[MAX_REPLY_SZ]; // When there's something in here, it seems it needs \r\0\r\n[reply]\n\n
} __attribute__((packed));

void set_atfwd_runtime_default();
void set_adb_runtime(bool mode);
void build_atcommand_reg_request(int tid, const char *command, char *buf);
int set_audio_profile(uint8_t io, uint8_t mode, uint8_t fsync, uint8_t clock,
                      uint8_t format, uint8_t sample, uint8_t num_slots,
                      uint8_t slot);

void *start_atfwd_thread();
char *get_adsp_version();
#define GET_ADSP_VER_CMD "AT+QGMR"
#endif
