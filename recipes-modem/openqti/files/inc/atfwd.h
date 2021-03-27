/* SPDX-License-Identifier: MIT */
#ifndef _ATFWD_H_
#define _ATFWD_H_
#include <stdbool.h>
#include <stdint.h>


static const struct {
	unsigned int command_id;
	const char *cmd;
} at_commands[] = {
	{0, "+QNAND", },
	{1, "+QPRTPARA", },
	{2, "+QWSERVER", },
	{3, "+QWTOCLIEN", },
	{4, "+QWTOCLI", },
	{5, "+QWPARAM", },
	{6, "+QDATAFWD", },
	{7, "+QFTCMD", },
	{8, "+QBTPWR", },
	{9, "+QBTLEADDR", },
	{10, "+QBTNAME", },
	{11, "+QBTGATREG", },
	{12, "+QBTGATSS", },
	{13, "+QBTGATSC", },
	{14, "+QBTGATSD", },
	{15, "+QBTGATSIND", },
	{16, "+QBTGATSNOD", },
	{17, "+QBTGATRRSP", },
	{18, "+QBTGATWRSP", },
	{19, "+QBTGATSENLE", },
	{20, "+QBTGATADV", },
	{21, "+QBTGATDISC", },
	{22, "+QBTGATPER", },
	{23, "+QBTGATDBALC", },
	{24, "+QBTGATDBDEALC", },
	{25, "+QBTGATSA", },
	{26, "+QBTGATDA", },
	{27, "+QFCT", },
	{28, "+QFCTTX", },
	{29, "+QFCTRX", },
	{30, "+QBTSPPACT", },
	{31, "+QBTSPPDIC", },
	{32, "+QBTSPPWRS", },
	{33, "+QBTSCAN", },
	{34, "+QBTAVACT", },
	{35, "+QBTAVREG", },
	{36, "+QBTAVCON", },
	{37, "+QBTHFGCON", },
	{38, "+QIIC", },
	{39, "+QAUDLOOP", },
	{40, "+QDAI", },
	{41, "+QSIDET", },
	{42, "+QAUDMOD", },
	{43, "+QEEC", },
	{44, "+QMIC", },
	{45, "+QRXGAIN", },
	{46, "+QAUDRD", },
	{47, "+QAUDPLAY", },
	{48, "+QAUDSTOP", },
	{49, "+QPSND", },
	{50, "+QTTS", },
	{51, "+QTTSETUP", },
	{52, "+QLTONE", },
	{53, "+QLDTMF", },
	{54, "+QAUDCFG", },
	{55, "+QTONEDET", },
	{56, "+QWTTS", },
	{57, "+QPCMV", },
	{58, "+QTXIIR", },
	{59, "+QRXIIR", },
	{60, "+QPOWD", },
	{61, "+QSCLK", },
	{62, "+QCFG", },
	{63, "+QADBKEY", },
	{64, "+QADC", },
	{65, "+QADCTEMP", },
	{66, "+QGPSCFG", },
	{67, "+QODM", },
	{68, "+QFUMO", },
	{69, "+QFUMOCFG", },
	{70, "+QPRINT", },
	{71, "+QSDMOUNT", },
	{72, "+QFASTBOOT", },
	{73, "+QPSM", },
	{74, "+QPSMCFG", },
	{75, "+QLINUXCPU", },
	{76, "+QVERSION", },
	{77, "+QSUBSYSVER", },
	{78, "+QTEMPDBG", },
	{79, "+QTEMP", },
	{80, "+QTEMPDBGLVL", },
	{81, "+QDIAGPORT", },
	{82, "+QLPMCFG", },
	{83, "+QSGMIICFG", },
	{84, "+QWWAN", },
	{85, "+QLWWANUP", },
	{86, "+QLWWANDOWN", },
	{87, "+QLWWANSTATUS", },
	{88, "+QLWWANURCCFG", },
	{89, "+QLWWANCID", },
	{90, "+QLPING", },
	{91, "+QWIFI", },
	{92, "+QWSSID", },
	{93, "+QWSSIDHEX", },
	{94, "+QWAUTH", },
	{95, "+QWMOCH", },
	{96, "+QWISO", },
	{97, "+QWBCAST", },
	{98, "+QWCLICNT", },
	{99, "+QWCLIP", },
	{100, "+QWCLILST", },
	{101, "+QWSTAINFO", },
	{102, "+QWCLIRM", },
	{103, "+QWSETMAC", },
	{104, "+QWRSTD", },
	{105, "+QWIFICFG", },
	{106, "+QAPRDYIND", },
	{107, "+QFOTADL", },
	{108, "+HEREWEGO", },
	{109, "+PINEROCKS"},
	{110, "+CFUN"},
	{111, "$QCPWRDN"},
	{112, "+CMUX"},
	{113, "+IPR"},
};

struct atcmd_reg_request {
	// QMI Header
	uint8_t ctlid; // 0x00
	uint16_t transaction_id; // incremental counter for each request
	uint16_t msgid; // 0x20 0x00
    uint16_t packet_size; // Sizeof the entire packet
	
	// the request packet itself
	// Obviously unfinished :)
	uint8_t dummy1; // always 0x00 0x01
	uint8_t var1; //0x0a - 0x0f || 0x11 - 0x13
	uint16_t dummy2; // always 0x00 0x01
	uint8_t var2; // 0x07 - 0x0e
	uint16_t dummy3; // always 0x00 0x01
	uint8_t command_length;// var3;
} __attribute__((packed));

struct at_command_modem_response {
	// QMI Header
	uint8_t ctlid; // 0x04
	uint16_t transaction_id; // incremental counter for each request, why 1 in its response? separate counter?
	uint16_t msgid; // 0x21 0x00// RESPONSE!
	uint16_t length; // 0x71 0x00 Sizeof the entire packet

	// the request packet itself
	// Obviously unfinished :)
	uint8_t dummy1; // always 0x00 0x01
	uint8_t var1; // 0x12?
	uint16_t dummy2; // always 0x00 0x01
	uint16_t dummy3; // always 0x00 0x00
	uint16_t dummy4; // always 0x00 0x01
  	uint16_t dummy5; // 0x00 0x00
	uint8_t cmd_len; // 0x09?
  	char *command;

  // The rest I have no idea yet, more sample packets are needed..
} __attribute__((packed));

struct at_command_simple_reply {
	uint8_t ctlid; // 0x04
	uint16_t transaction_id; // incremental counter for each request, why 1 in its response? separate counter?
	uint16_t msgid; // 0x21 0x00// RESPONSE!
	uint16_t length; // 0x71 0x00 Sizeof the entire packet
	uint32_t command;
	uint16_t result;
	uint16_t response;
	unsigned char *raw_response;
};

#endif