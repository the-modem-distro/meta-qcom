// SPDX-License-Identifier: MIT
#ifndef _CELL_H_
#define _CELL_H_

#include "../inc/openqti.h"
#include <stdbool.h>
#include <stdio.h>

#define GET_SERVING_CELL "AT+QENG=\"servingcell\"\r\n"
#define GET_NEIGHBOUR_CELL "AT+QENG=\"neighbourcell\"\r\n"
#define GET_QENG_RESPONSE_PROTO "+QENG:"
#define GET_COMMON_IND "AT+CIND?\r\n"
#define GET_COMMON_IND_RESPONSE_PROTO "+CIND:"
#define GET_IMSI "AT+CIMI\r\n"

struct gsm_neighbour {
  int arfcn;
  int cell_resel_priority;
  int thresh_gsm_high;
  int thresh_gsm_low;
  int ncc_permitted;
  int band;
  int bsic_id;
  int rssi;
  int srxlev;
};

struct gsm_data {
  char lac[4];          // only GSM/WCDMA
  int bsic;             // only in GSM
  int arfcn;
  int band;
  int rxlev;
  int txp;
  int rla;
  int drx;
  int c1;
  int c2;
  int gprs;
  int tch;
  int ts;
  int ta;
  int maio;
  int hsn;
  int rxlevsub;
  int rxlevfull;
  int rxqualsub;
  int rxqualfull;
  int voicecodec;
  uint8_t neighbour_sz;
  struct gsm_neighbour neighbours[8];
};
/*
<uarfcn>,<cell_resel_priorit
y>,<thresh_Xhigh>,<thresh_Xlow>,<psc>,<cpich_rscp>,
<cpich_ecno>,<srxlev>
*/
struct wcdma_neighbour {
  int uarfcn;
  int cell_resel_priority;
  int thresh_Xhigh;
  int thresh_Xlow;
  int psc;
  int cpich_rscp;
  int cpich_ecno;
  int srxlev;
};

struct wcdma_data {
  char lac[4];          // only GSM/WCDMA
  int uarfcn;
  int psc;
  int rac;
  int rscp;
  int ecio;
  int phych;
  int sf;
  int slot;
  int speech_codec;
  int conmod;
  uint8_t neighbour_sz;
  struct wcdma_neighbour neighbours[8];
};


struct lte_neighbour {
  bool is_intra; // is intrafrequency or not?
  int earfcn;
  int pcid;
  int rsrq;
  int rsrp;
  int rssi;
  int sinr;
  int srxlev;
  int cell_resel_priority; // up here intra
  int s_non_intra_search;
  int thresh_serving_low;
  int s_intra_search;
};

struct lte_data {
  int is_tdd;
  int pcid;
  int earfcn;
  int freq_band_ind;
  int ul_bandwidth;
  int dl_bandwidth;
  int tac;
  int rsrp;
  int rsrq;
  int rssi;
  int sinr;
  int srxlev;
  uint8_t neighbour_sz;
  struct lte_neighbour neighbours[16];
};


struct cell_report {
  char cmd_id[16];      // servingcell || neighbourcell
  char state[16];       // NOCONN
  char network_mode[6]; // GSM  || WCDMA || LTE
  int mcc;              // Country code
  int mnc;              // Network code
  char cell_id[16];        // Current Cell ID
  int net_type; // 0 GSM || 1 WCDMA || 2 LTE
  struct gsm_data gsm;
  struct wcdma_data wcdma;
  struct lte_data lte;
};

struct network_state {
  uint8_t network_type; // LTE / WCDMA / GSM / ??
  uint8_t signal_level; // in dB
  int signal_bars;      // 0 - 5
  int in_service;       // 0 || 1
  int in_call;          // 0 || 1
  int is_roaming;       // 0 || 1
  int ps_domain;        // 0 || 1
};

static const char *network_types[] = {
    "Unknown", "CDMA", "EVDO", "AMPS", "GSM", "UMTS", "Error", "Error", "LTE"};

uint8_t get_network_type();
uint8_t get_signal_strength();
void update_network_data(uint8_t network_type, uint8_t signal_level);
struct network_state get_network_status();
struct cell_report get_current_cell_report();
#endif
