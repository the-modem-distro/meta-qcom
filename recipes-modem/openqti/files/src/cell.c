// SPDX-License-Identifier: MIT

#include "../inc/config.h"
#include "../inc/cell.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/logger.h"
#include "../inc/sms.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_RESPONSE_SZ 4096
/*
 * cell data
 *  We're going to exploit Quectel's engineering commands
 *  to track signal status, network mode and servicing and
 *  neighbour cells
 *
 *  If we get a sudden change in neighbour cells, service drop
 *  etc. we should be able to track it down here
 */

struct network_state net_status;

struct {
  uint8_t history_sz;
  struct cell_report current_report;
  struct cell_report history[128];
} report_data;

/*
Return last reported network type
0x00 -> No service
0x01 -> CDMA
0x02 -> CDMA EVDO
0x03 -> AMPS
0x04 -> GSM
0x05 -> UMTS
0x06 ??
0x07 ??
0x08 LTE
*/

uint8_t get_network_type() { return net_status.network_type; }

/* Returns last reported signal in %, based on signal bars */
uint8_t get_signal_strength() {
  if (net_status.signal_bars > 0) {
    return net_status.signal_bars * 5 / 100;
  }

  return 0;
}
int is_network_in_service() {
  return net_status.in_service;
}

struct network_state get_network_status() {
  return net_status;
}

struct cell_report get_current_cell_report() {
  return report_data.current_report;
}

struct cell_report parse_report_data(char *orig_string) {
  struct cell_report report;
  char delim[] = ",";
  char str[MAX_RESPONSE_SZ];
  int ret;
  strcpy(str, (char *)orig_string);
  int init_size = strlen(str);
  int positions[128];
  char slices[64][MAX_RESPONSE_SZ];
  int cur_word = 1;
  int cur_word_id = -1;
  int start;
  int end;
  positions[0] = 0;
  char *ptr = strtok(str, delim);
  while (ptr != NULL) {
    // logger(MSG_INFO, "'%s: %s'\n", __func__, ptr);
    ptr = strtok(NULL, delim);
  }

  for (int i = 0; i < init_size; i++) {
    if (str[i] == 0) {
      positions[cur_word] = i;
      cur_word++;
    }
  }
  for (int i = 0; i < cur_word; i++) {
    start = positions[i];
    if (i + 1 >= cur_word) {
      end = init_size;
    } else {
      end = positions[i + 1];
    }
    if (i > 0) {
      start++;
    }
    memset(slices[i], 0, MAX_RESPONSE_SZ);
    memcpy(slices[i], str + start, (end - start));
    if (strcmp(slices[i], "-") == 0) {
      strncpy(slices[i], "-999", strlen("-999"));
      // If it is empty set it to -999 so we can use that info later
      // Otherwise strtol will convert "-" to 9
    }

    slices[i][strlen(slices[i])] = '\0';
    /*
    Now, go filling the blanks by position */
  }
  report.net_type = -1;
  if (strstr(orig_string, "GSM") != NULL) {
    logger(MSG_INFO, "%s GSM network data report\n", __func__);
    report.net_type = 0;
    report.mcc = strtol(slices[3], NULL, 10);
    report.mnc = strtol(slices[4], NULL, 10);
    strncpy(report.gsm.lac, slices[5], strlen(slices[5]));
    strncpy(report.cell_id, slices[6], strlen(slices[6]));
    report.gsm.bsic = strtol(slices[7], NULL, 10);
    report.gsm.arfcn = strtol(slices[8], NULL, 10);
    report.gsm.band = strtol(slices[9], NULL, 10);
    report.gsm.rxlev = strtol(slices[10], NULL, 10);
    report.gsm.txp = strtol(slices[11], NULL, 10);
    report.gsm.rla = strtol(slices[12], NULL, 10);
    report.gsm.drx = strtol(slices[13], NULL, 10);
    report.gsm.c1 = strtol(slices[14], NULL, 10);
    report.gsm.c2 = strtol(slices[15], NULL, 10);
    report.gsm.gprs = strtol(slices[16], NULL, 10);
    report.gsm.tch = strtol(slices[17], NULL, 10);
    report.gsm.ts = strtol(slices[18], NULL, 10);
    report.gsm.ta = strtol(slices[19], NULL, 10);
    report.gsm.maio = strtol(slices[20], NULL, 10);
    report.gsm.hsn = strtol(slices[21], NULL, 10);
    report.gsm.rxlevsub = strtol(slices[22], NULL, 10);
    report.gsm.rxlevfull = strtol(slices[23], NULL, 10);
    report.gsm.rxqualsub = strtol(slices[24], NULL, 10);
    report.gsm.rxqualfull = strtol(slices[25], NULL, 10);
    report.gsm.voicecodec = strtol(slices[26], NULL, 10);

  } else if (strstr(orig_string, "WCDMA") != NULL) {
    logger(MSG_INFO, "%s WCDMA network data report\n", __func__);
    report.net_type = 1;
    report.mcc = strtol(slices[3], NULL, 10);
    report.mnc = strtol(slices[4], NULL, 10);
    strncpy(report.wcdma.lac, slices[5], strlen(slices[5]));
    strncpy(report.cell_id, slices[6], strlen(slices[6]));
    report.wcdma.uarfcn = strtol(slices[7], NULL, 10);
    report.wcdma.psc = strtol(slices[8], NULL, 10);
    report.wcdma.rac = strtol(slices[9], NULL, 10);
    report.wcdma.rscp = strtol(slices[10], NULL, 10);
    report.wcdma.ecio = strtol(slices[11], NULL, 10);
    report.wcdma.phych = strtol(slices[12], NULL, 10);
    report.wcdma.sf = strtol(slices[13], NULL, 10);
    report.wcdma.slot = strtol(slices[14], NULL, 10);
    report.wcdma.speech_codec = strtol(slices[15], NULL, 10);
    report.wcdma.conmod = strtol(slices[16], NULL, 10);

  } else if (strstr(orig_string, "LTE") != NULL) {
    logger(MSG_INFO, "%s LTE network data report\n", __func__);
    report.net_type = 2;
    report.lte.is_tdd = strtol(slices[3], NULL, 10);
    report.mcc = strtol(slices[4], NULL, 10);
    report.mnc = strtol(slices[5], NULL, 10);
    strncpy(report.cell_id, slices[6], strlen(slices[6]));
    report.lte.pcid = strtol(slices[7], NULL, 10);
    report.lte.earfcn = strtol(slices[8], NULL, 10);
    report.lte.freq_band_ind = strtol(slices[9], NULL, 10);
    report.lte.ul_bandwidth = strtol(slices[10], NULL, 10);
    report.lte.dl_bandwidth = strtol(slices[11], NULL, 10);
    report.lte.tac = strtol(slices[12], NULL, 10);
    report.lte.rsrp = strtol(slices[13], NULL, 10);
    report.lte.rsrq = strtol(slices[14], NULL, 10);
    report.lte.rssi = strtol(slices[15], NULL, 10);
    report.lte.sinr = strtol(slices[16], NULL, 10);
    report.lte.srxlev = strtol(slices[17], NULL, 10);

  } else {
    logger(MSG_ERROR, "%s Unknown data: %s\n", __func__, orig_string);
  }

  return report;
}

void parse_lte_intra_neighbour_data(char *orig_string, int len) {
  struct lte_neighbour report;
  int i;
  char delim[] = ",";
  char str[MAX_RESPONSE_SZ];
  int ret;
  strncpy(str, (char *)orig_string, len);
  int init_size = strlen(str);
  int positions[128];
  char slices[64][MAX_RESPONSE_SZ];
  int cur_word = 1;
  int cur_word_id = -1;
  int start;
  int end;
  positions[0] = 0;
  char *ptr = strtok(str, delim);
  while (ptr != NULL) {
    // logger(MSG_INFO, "'%s: %s'\n", __func__, ptr);
    ptr = strtok(NULL, delim);
  }

  for (int i = 0; i < init_size; i++) {
    if (str[i] == 0) {
      positions[cur_word] = i;
      cur_word++;
    }
  }
  for (int i = 0; i < cur_word; i++) {
    start = positions[i];
    if (i + 1 >= cur_word) {
      end = init_size;
    } else {
      end = positions[i + 1];
    }
    if (i > 0) {
      start++;
    }
    memset(slices[i], 0, MAX_RESPONSE_SZ);
    memcpy(slices[i], str + start, (end - start));
    if (strcmp(slices[i], "-") == 0) {
      strncpy(slices[i], "-999", strlen("-999"));
      // If it is empty set it to -999 so we can use that info later
      // Otherwise strtol will convert "-" to 9
    }

    slices[i][strlen(slices[i])] = '\0';
    /*
    Now, go filling the blanks by position */
  }
  logger(MSG_INFO, "%s LTE neighbourcell intrafrequency cell data report\n",
         __func__);
  if (cur_word > 7) {
    report.is_intra = true;
    report.earfcn = strtol(slices[2], NULL, 10);
    report.pcid = strtol(slices[3], NULL, 10);
    report.rsrq = strtol(slices[4], NULL, 10);
    report.rsrp = strtol(slices[5], NULL, 10);
    report.rssi = strtol(slices[6], NULL, 10);
    report.sinr = strtol(slices[7], NULL, 10);
    if (cur_word > 8)
      report.srxlev = strtol(slices[8], NULL, 10);
    if (cur_word > 9)
      report.cell_resel_priority = strtol(slices[9], NULL, 10);
    if (cur_word > 10)
      report.s_non_intra_search = strtol(slices[10], NULL, 10);
    if (cur_word > 11)
      report.thresh_serving_low = strtol(slices[11], NULL, 10);
    if (cur_word > 12)
      report.s_non_intra_search = strtol(slices[12], NULL, 10);
    if (cur_word > 13)
      report.thresh_serving_low = strtol(slices[13], NULL, 10);
    if (cur_word > 14)
      report.s_intra_search = strtol(slices[14], NULL, 10);

    if (report_data.current_report.lte.neighbour_sz == 15) {
      logger(MSG_INFO, "%s: Need to rotate neighbour log\n", __func__);
      // rotate
      for (i = 1; i < 16; i++) {
        report_data.current_report.lte.neighbours[i - 1] =
            report_data.current_report.lte.neighbours[i];
      }
      report_data.current_report.lte
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
    } else {
      report_data.current_report.lte
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
      report_data.current_report.lte.neighbour_sz++;
    }
  } else {
    logger(MSG_WARN, "%s: Not enough data\n", __func__);
  }
}

void parse_lte_inter_neighbour_data(char *orig_string, int len) {
  struct lte_neighbour report;
  int i;
  char delim[] = ",";
  char str[MAX_RESPONSE_SZ];
  int ret;
  strncpy(str, (char *)orig_string, len);
  int init_size = strlen(str);
  int positions[128];
  char slices[64][MAX_RESPONSE_SZ];
  int cur_word = 1;
  int cur_word_id = -1;
  int start;
  int end;
  positions[0] = 0;
  char *ptr = strtok(str, delim);
  while (ptr != NULL) {
    // logger(MSG_INFO, "'%s: %s'\n", __func__, ptr);
    ptr = strtok(NULL, delim);
  }

  for (int i = 0; i < init_size; i++) {
    if (str[i] == 0) {
      positions[cur_word] = i;
      cur_word++;
    }
  }
  for (int i = 0; i < cur_word; i++) {
    start = positions[i];
    if (i + 1 >= cur_word) {
      end = init_size;
    } else {
      end = positions[i + 1];
    }
    if (i > 0) {
      start++;
    }
    memset(slices[i], 0, MAX_RESPONSE_SZ);
    memcpy(slices[i], str + start, (end - start));
    if (strcmp(slices[i], "-") == 0) {
      strncpy(slices[i], "-999", strlen("-999"));
      // If it is empty set it to -999 so we can use that info later
      // Otherwise strtol will convert "-" to 9
    }

    slices[i][strlen(slices[i])] = '\0';
    /*
    Now, go filling the blanks by position */
  }
  logger(MSG_INFO, "%s LTE neighbourcell inter frequency cell data report\n",
         __func__);
  if (cur_word > 13) {
    report.is_intra = false;
    report.earfcn = strtol(slices[2], NULL, 10);
    report.pcid = strtol(slices[3], NULL, 10);
    report.rsrq = strtol(slices[4], NULL, 10);
    report.rsrp = strtol(slices[5], NULL, 10);
    report.rssi = strtol(slices[6], NULL, 10);
    report.sinr = strtol(slices[7], NULL, 10);
    if (cur_word > 8)
      report.srxlev = strtol(slices[8], NULL, 10);
    if (cur_word > 9)
      report.cell_resel_priority = strtol(slices[9], NULL, 10);
    if (cur_word > 10)
      report.s_non_intra_search = strtol(slices[10], NULL, 10);
    if (cur_word > 11)
      report.thresh_serving_low = strtol(slices[11], NULL, 10);
    if (cur_word > 12)
      report.s_non_intra_search = strtol(slices[12], NULL, 10);
    if (cur_word > 13)
      report.thresh_serving_low = strtol(slices[13], NULL, 10);
    if (cur_word > 14)
      report.s_intra_search = strtol(slices[14], NULL, 10);

    if (report_data.current_report.lte.neighbour_sz == 15) {
      logger(MSG_INFO, "%s: Need to rotate neighbour log\n", __func__);
      // rotate
      for (i = 1; i < 16; i++) {
        report_data.current_report.lte.neighbours[i - 1] =
            report_data.current_report.lte.neighbours[i];
      }
      report_data.current_report.lte
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
    } else {
      report_data.current_report.lte
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
      report_data.current_report.lte.neighbour_sz++;
    }
  } else {
    logger(MSG_WARN, "%s: Not enough data\n", __func__);
  }
}

void parse_wcdma_neighbour_data(char *orig_string, int len) {
  struct wcdma_neighbour report;
  int i;
  char delim[] = ",";
  char str[MAX_RESPONSE_SZ];
  int ret;
  strncpy(str, (char *)orig_string, len);
  int init_size = strlen(str);
  int positions[128];
  char slices[64][MAX_RESPONSE_SZ];
  int cur_word = 1;
  int cur_word_id = -1;
  int start;
  int end;
  positions[0] = 0;
  char *ptr = strtok(str, delim);
  while (ptr != NULL) {
    // logger(MSG_INFO, "'%s: %s'\n", __func__, ptr);
    ptr = strtok(NULL, delim);
  }

  for (int i = 0; i < init_size; i++) {
    if (str[i] == 0) {
      positions[cur_word] = i;
      cur_word++;
    }
  }
  for (int i = 0; i < cur_word; i++) {
    start = positions[i];
    if (i + 1 >= cur_word) {
      end = init_size;
    } else {
      end = positions[i + 1];
    }
    if (i > 0) {
      start++;
    }
    memset(slices[i], 0, MAX_RESPONSE_SZ);
    memcpy(slices[i], str + start, (end - start));
    if (strcmp(slices[i], "-") == 0) {
      strncpy(slices[i], "-999", strlen("-999"));
      // If it is empty set it to -999 so we can use that info later
      // Otherwise strtol will convert "-" to 9
    }

    slices[i][strlen(slices[i])] = '\0';
    /* Now, go filling the blanks by position */
  }
  logger(MSG_INFO, "%s WCDMA neighbour cell data report\n", __func__);
  if (cur_word > 8) {
    report.uarfcn = strtol(slices[2], NULL, 10);
    report.cell_resel_priority = strtol(slices[3], NULL, 10);
    report.thresh_Xhigh = strtol(slices[4], NULL, 10);
    report.thresh_Xlow = strtol(slices[5], NULL, 10);
    report.psc = strtol(slices[6], NULL, 10);
    report.cpich_rscp = strtol(slices[7], NULL, 10);
    report.cpich_ecno = strtol(slices[8], NULL, 10);
    report.srxlev = strtol(slices[9], NULL, 10);

    if (report_data.current_report.wcdma.neighbour_sz == 15) {
      logger(MSG_INFO, "%s: Need to rotate neighbour log\n", __func__);
      // rotate
      for (i = 1; i < 16; i++) {
        report_data.current_report.wcdma.neighbours[i - 1] =
            report_data.current_report.wcdma.neighbours[i];
      }
      report_data.current_report.wcdma
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
    } else {
      report_data.current_report.wcdma
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
      report_data.current_report.wcdma.neighbour_sz++;
    }
  } else {
    logger(MSG_WARN, "%s: Not enough data\n", __func__);
  }
}

void parse_gsm_neighbour_data(char *orig_string, int len) {
  struct gsm_neighbour report;
  int i;
  char delim[] = ",";
  char str[MAX_RESPONSE_SZ];
  int ret;
  strncpy(str, (char *)orig_string, len);
  int init_size = strlen(str);
  int positions[128];
  char slices[64][MAX_RESPONSE_SZ];
  int cur_word = 1;
  int cur_word_id = -1;
  int start;
  int end;
  positions[0] = 0;
  char *ptr = strtok(str, delim);
  while (ptr != NULL) {
    // logger(MSG_INFO, "'%s: %s'\n", __func__, ptr);
    ptr = strtok(NULL, delim);
  }

  for (int i = 0; i < init_size; i++) {
    if (str[i] == 0) {
      positions[cur_word] = i;
      cur_word++;
    }
  }
  for (int i = 0; i < cur_word; i++) {
    start = positions[i];
    if (i + 1 >= cur_word) {
      end = init_size;
    } else {
      end = positions[i + 1];
    }
    if (i > 0) {
      start++;
    }
    memset(slices[i], 0, MAX_RESPONSE_SZ);
    memcpy(slices[i], str + start, (end - start));
    if (strcmp(slices[i], "-") == 0) {
      strncpy(slices[i], "-999", strlen("-999"));
      // If it is empty set it to -999 so we can use that info later
      // Otherwise strtol will convert "-" to 9
    }

    slices[i][strlen(slices[i])] = '\0';
    /* Now, go filling the blanks by position */
  }
  logger(MSG_INFO, "%s GSM neighbour cell data report\n", __func__);
  if (cur_word > 9) {
    report.arfcn = strtol(slices[2], NULL, 10);
    report.cell_resel_priority = strtol(slices[3], NULL, 10);
    report.thresh_gsm_high = strtol(slices[4], NULL, 10);
    report.thresh_gsm_low = strtol(slices[5], NULL, 10);
    report.ncc_permitted = strtol(slices[6], NULL, 10);
    report.band = strtol(slices[7], NULL, 10);
    report.bsic_id = strtol(slices[8], NULL, 10);
    report.rssi = strtol(slices[9], NULL, 10);
    report.srxlev = strtol(slices[10], NULL, 10);

    if (report_data.current_report.wcdma.neighbour_sz == 15) {
      logger(MSG_INFO, "%s: Need to rotate neighbour log\n", __func__);
      // rotate
      for (i = 1; i < 16; i++) {
        report_data.current_report.gsm.neighbours[i - 1] =
            report_data.current_report.gsm.neighbours[i];
      }
      report_data.current_report.gsm
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
    } else {
      report_data.current_report.gsm
          .neighbours[report_data.current_report.lte.neighbour_sz] = report;
      report_data.current_report.gsm.neighbour_sz++;
    }
  } else {
    logger(MSG_WARN, "%s: Not enough data\n", __func__);
  }
}

/* Connect to the AT port, send a command, and get a response */
int get_data_from_command(char *command, size_t len, char *expected_response,
                          char *response) {
  int fd, ret;
  int fnret;
  fd_set readfds;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  fd = open(SMD_SEC_AT, O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open SMD10 entry\n", __func__);
    return -EINVAL;
  }
  ret = write(fd, command, len);
  FD_SET(fd, &readfds);
  ret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, response, MAX_RESPONSE_SZ);
    if (strstr(response, expected_response) != NULL) {
      fnret = 0;
    }
  } else {
    logger(MSG_ERROR, "%s: Missed our window\n", __func__);
    fnret = -EBADMSG;
  }

  close(fd);
  return fnret;
}
char *get_report_network_type(int type) {
  if (type == 0) {
    return "GSM";
  } else if (type == 1) {
    return "WCDMA";
  } else if (type == 2) {
    return "LTE";
  }

  return "UNKNOWN";
}
/* Analyze the data in the reports */
void analyze_data() {
  bool do_send = false;
  int strsz = 0;
  uint8_t *reply = calloc(256, sizeof(unsigned char));
  int i;
  logger(MSG_INFO, "%s: There are %i entries in the log\n", __func__,
         report_data.history_sz);
  if (report_data.history_sz < 5) {
    logger(MSG_INFO, "%s: Not enough data has been retrieved yet.\n", __func__);
    free(reply);
    reply = NULL;
    return;
  }
  for (i = report_data.history_sz; i > (report_data.history_sz - 1); i--) {
    logger(MSG_INFO, "%s: Checking report %i\n", __func__, i);
    if (report_data.history[i].net_type !=
        report_data.history[i - 1].net_type) {
      strsz +=
          snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                   "Network mode changed: %s -> %s\n",
                   get_report_network_type(report_data.history[i - 1].net_type),
                   get_report_network_type(report_data.history[i].net_type));
      do_send = true;
    } else {
      strsz +=
          snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                   "Network mode maintained: %s \n",
                   get_report_network_type(report_data.history[i].net_type));
    }

    if (strcmp(report_data.history[i].cell_id,
               report_data.history[i - 1].cell_id) != 0) {
      strsz += snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                        "Cell ID Changed: %s -> %s\n",
                        report_data.history[i - 1].cell_id,
                        report_data.history[i].cell_id);
      do_send = true;
    } else {
      strsz +=
          snprintf((char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
                   "Cell ID maintained %s \n", report_data.history[i].cell_id);
    }
    switch (report_data.history[i].net_type) {
    case 0:
      strsz += snprintf(
          (char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
          "rxlev %i\nsrxlevfull%i\nlac %s", report_data.history[i].gsm.rxlevsub,
          report_data.history[i].gsm.rxlevfull, report_data.history[i].gsm.lac);
      break;
    case 1:
      strsz += snprintf(
          (char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
          "rscp %i\necio %i\nlac %s", report_data.history[i].wcdma.rscp,
          report_data.history[i].wcdma.ecio, report_data.history[i].wcdma.lac);
      break;
    case 2:
      strsz += snprintf(
          (char *)reply + strsz, MAX_MESSAGE_SIZE - strsz,
          "rssi %i\nsrxlev %i\nsnr %i", report_data.history[i].lte.rssi,
          report_data.history[i].lte.srxlev, report_data.history[i].lte.sinr);
      break;
    }
  }
  if (do_send) {
    add_message_to_queue(reply, strsz);
  }
  free(reply);
  reply = NULL;
}

/* Data retrieval functions */
void read_neighbour_cells() {
  int ret = 0;
  char *pt;
  char *response;
  char *start, *end;
  unsigned count = 0;
  response = malloc(MAX_RESPONSE_SZ * sizeof(char));
  int command_length = strlen(GET_NEIGHBOUR_CELL);

  logger(MSG_INFO, "%s: Read neighbour cell start\n", __func__);
  ret = get_data_from_command(GET_NEIGHBOUR_CELL, command_length,
                              GET_QENG_RESPONSE_PROTO, response);
  if (ret != 0) {
    logger(MSG_ERROR, "%s: Command %s failed. Response: %s\n", __func__,
           GET_NEIGHBOUR_CELL, response);
  } else {
    start = end = (char *)response;
    // Need to loop through every line (size of the response will vary to
    // network conditions)
    while ((end = strchr(start, '\n'))) {
      if (strstr(start, "neighbourcell intra") != NULL) {
        logger(MSG_DEBUG, "%s: Read LTE INTRA -> %s \n", __func__, start);
        parse_lte_intra_neighbour_data(start, (int)(end - start + 1));

      } else if (strstr(start, "neighbourcell inter") != NULL) {
        logger(MSG_DEBUG, "%s: Read LTE INTER %s\n", __func__, start);
        parse_lte_inter_neighbour_data(start, (int)(end - start + 1));

      } else if (strstr(start, "WCDMA") != NULL) {
        logger(MSG_DEBUG, "%s: Read WCDMA %s\n", __func__, start);
        parse_wcdma_neighbour_data(start, (int)(end - start + 1));

      } else if (strstr(start, "GSM") != NULL) {
        logger(MSG_DEBUG, "%s: Read GSM %s\n", __func__, start);
        parse_gsm_neighbour_data(start, (int)(end - start + 1));

      } else if (strstr(start, "OK") != NULL) {
        logger(MSG_DEBUG, "%s: Report end %s\n", __func__, start);

      } else {
        logger(MSG_INFO, "%s: Unknown report type: %s\n", __func__, start);
      }

      start = end + 1;
    }
  }
  free(response);
  response = NULL;
}

void read_serving_cell() {
  int ret = 0;
  int i;
  char *pt;
  char *response;
  response = malloc(MAX_RESPONSE_SZ * sizeof(char));
  int command_length = strlen(GET_SERVING_CELL);

  logger(MSG_INFO, "%s: Read serving cell start\n", __func__);
  ret = get_data_from_command(GET_SERVING_CELL, command_length,
                              GET_QENG_RESPONSE_PROTO, response);
  if (ret != 0) {
    logger(MSG_ERROR, "%s: Command %s failed. Response: %s\n", __func__,
           GET_SERVING_CELL, response);
  } else {
    logger(MSG_INFO, "%s: Command %s succeeded! Response: %s\n", __func__,
           GET_SERVING_CELL, response);
    if (strlen(response) > 18) {
      report_data.current_report = parse_report_data(response);
      read_neighbour_cells();
      if (report_data.history_sz > 126) {
        for (i = 1; i < 128; i++) {
          report_data.history[i - 1] = report_data.history[i];
        }
      } else {
        report_data.history_sz++;
      }
      report_data.history[report_data.history_sz] = report_data.current_report;
    }
  }
  free(response);
  response = NULL;
  analyze_data();
}

void read_at_cind() {
  char *response;
  response = malloc(MAX_RESPONSE_SZ * sizeof(char));
  int command_length = strlen(GET_COMMON_IND);
  int ret = 0;
  logger(MSG_DEBUG, "%s: Read CIND start\n", __func__);

  ret = get_data_from_command(GET_COMMON_IND, command_length,
                              GET_COMMON_IND_RESPONSE_PROTO, response);

  if (ret != 0) {
    logger(MSG_ERROR, "%s: Command %s failed. Response: %s\n", __func__,
           GET_COMMON_IND, response);
  } else {
    logger(MSG_DEBUG, "%s: Command %s succeeded! Response: %s\n", __func__,
           GET_COMMON_IND, response);
    if (strlen(response) > 18) {
      net_status.signal_bars = get_int_from_str(response, 11);
      net_status.in_service = get_int_from_str(response, 13);
      net_status.in_call = get_int_from_str(response, 15);
      net_status.is_roaming = get_int_from_str(response, 17);
      net_status.ps_domain = get_int_from_str(response, 21);
    }
  }
  free(response);
  response = NULL;
}

void update_network_data(uint8_t network_type, uint8_t signal_level) {
  logger(MSG_INFO, "%s: update network data\n", __func__);
  net_status.network_type = network_type;
  net_status.signal_level = signal_level;
  logger(MSG_DEBUG, "%s: Request AT+CIND\n", __func__);
  read_at_cind();
  if (is_signal_tracking_enabled()) {
    logger(MSG_INFO, "%s: Read serving and neighbour cell data\n", __func__);
    read_serving_cell();
  }
}