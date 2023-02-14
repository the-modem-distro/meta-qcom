// SPDX-License-Identifier: MIT
#ifndef __NAS_H__
#define __NAS_H__

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>



#define INTERNAL_CELLID_INFO_PATH "/persist/cellid_data.raw"
#define MAX_REPORT_NUM 4096
#define MAX_FILE_SIZE 13107200

/*
 * Headers for the Network Access Service
 *
 *
 *
 */

enum {
  NAS_SERVICE_PROVIDER_NAME = 0x10,
  NAS_OPERATOR_PLMN_LIST = 0x11,
  NAS_OPERATOR_PLMN_NAME = 0x12,
  NAS_OPERATOR_STRING_NAME = 0x13,
  NAS_NITZ_INFORMATION = 0x14,
  NAS_PREFERRED_NETWORKS = 0x10,
  NAS_RESET = 0x0000,
  NAS_ABORT = 0x0001,
  NAS_SET_EVENT_REPORT = 0x0002,
  NAS_EVENT_REPORT = 0x0002,
  NAS_REGISTER_INDICATIONS = 0x0003,
  NAS_GET_SUPPORTED_MESSAGES = 0x001E,
  NAS_GET_SIGNAL_STRENGTH = 0x0020,
  NAS_NETWORK_SCAN = 0x0021,
  NAS_INITIATE_NETWORK_REGISTER = 0x0022,
  NAS_ATTACH_DETACH = 0x0023,
  NAS_GET_SERVING_SYSTEM = 0x0024,
  NAS_SERVING_SYSTEM = 0x0024,
  NAS_GET_HOME_NETWORK = 0x0025,
  NAS_GET_PREFERRED_NETWORKS = 0x0026,
  NAS_SET_PREFERRED_NETWORKS = 0x0027,
  NAS_SET_TECHNOLOGY_PREFERENCE = 0x002A,
  NAS_GET_TECHNOLOGY_PREFERENCE = 0x002B,
  NAS_GET_RF_BAND_INFORMATION = 0x0031,
  NAS_SET_SYSTEM_SELECTION_PREFERENCE = 0x0033,
  NAS_GET_SYSTEM_SELECTION_PREFERENCE = 0x0034,
  NAS_GET_OPERATOR_NAME = 0x0039,
  NAS_OPERATOR_NAME = 0x003A,
  NAS_GET_CELL_LOCATION_INFO = 0x0043,
  NAS_GET_PLMN_NAME = 0x0044,
  NAS_NETWORK_TIME = 0x004C,
  NAS_GET_SYSTEM_INFO = 0x004D,
  NAS_SYSTEM_INFO = 0x004E,
  NAS_GET_SIGNAL_INFO = 0x004F,
  NAS_CONFIG_SIGNAL_INFO = 0x0050,
  NAS_CONFIG_SIGNAL_INFO_V2 = 0x006C,
  NAS_SIGNAL_INFO = 0x0051,
  NAS_GET_TX_RX_INFO = 0x005A,
  NAS_GET_CDMA_POSITION_INFO = 0x0065,
  NAS_FORCE_NETWORK_SEARCH = 0x0067,
  NAS_NETWORK_REJECT = 0x0068,
  NAS_GET_DRX = 0x0089,
  NAS_GET_LTE_CPHY_CA_INFO = 0x00AC,
  NAS_GET_IMS_PREFERENCE_STATE = 0x0073,
  NAS_SWI_GET_STATUS = 0x5556,
};

static const struct {
  uint16_t id;
  const char *cmd;
} nas_svc_commands[] = {
    {NAS_SERVICE_PROVIDER_NAME, "Service Provider Name"},
    {NAS_OPERATOR_PLMN_LIST, "Operator PLMN List"},
    {NAS_OPERATOR_PLMN_NAME, "Operator PLMN Name"},
    {NAS_OPERATOR_STRING_NAME, "Operator String Name"},
    {NAS_NITZ_INFORMATION, "NITZ Information"},
    {NAS_PREFERRED_NETWORKS, "Preferred Networks"},
    {NAS_RESET, "Reset"},
    {NAS_ABORT, "Abort"},
    {NAS_SET_EVENT_REPORT, "Set Event Report"},
    {NAS_EVENT_REPORT, "Event Report"},
    {NAS_REGISTER_INDICATIONS, "Register Indications"},
    {NAS_GET_SUPPORTED_MESSAGES, "Get Supported Messages"},
    {NAS_GET_SIGNAL_STRENGTH, "Get Signal Strength"},
    {NAS_NETWORK_SCAN, "Network Scan"},
    {NAS_INITIATE_NETWORK_REGISTER, "Initiate Network Register"},
    {NAS_ATTACH_DETACH, "Attach Detach"},
    {NAS_GET_SERVING_SYSTEM, "Get Serving System"},
    {NAS_SERVING_SYSTEM, "Serving System"},
    {NAS_GET_HOME_NETWORK, "Get Home Network"},
    {NAS_GET_PREFERRED_NETWORKS, "Get Preferred Networks"},
    {NAS_SET_PREFERRED_NETWORKS, "Set Preferred Networks"},
    {NAS_SET_TECHNOLOGY_PREFERENCE, "Set Technology Preference"},
    {NAS_GET_TECHNOLOGY_PREFERENCE, "Get Technology Preference"},
    {NAS_GET_RF_BAND_INFORMATION, "Get RF Band Information"},
    {NAS_SET_SYSTEM_SELECTION_PREFERENCE, "Set System Selection Preference"},
    {NAS_GET_SYSTEM_SELECTION_PREFERENCE, "Get System Selection Preference"},
    {NAS_GET_OPERATOR_NAME, "Get Operator Name"},
    {NAS_OPERATOR_NAME, "Operator Name"},
    {NAS_GET_CELL_LOCATION_INFO, "Get Cell Location Info"},
    {NAS_GET_PLMN_NAME, "Get PLMN Name"},
    {NAS_NETWORK_TIME, "Network Time"},
    {NAS_GET_SYSTEM_INFO, "Get System Info"},
    {NAS_SYSTEM_INFO, "System Info"},
    {NAS_GET_SIGNAL_INFO, "Get Signal Info"},
    {NAS_CONFIG_SIGNAL_INFO, "Config Signal Info"},
    {NAS_CONFIG_SIGNAL_INFO_V2, "Config Signal Info v2"},
    {NAS_SIGNAL_INFO, "Signal Info"},
    {NAS_GET_TX_RX_INFO, "Get Tx Rx Info"},
    {NAS_GET_CDMA_POSITION_INFO, "Get CDMA Position Info"},
    {NAS_FORCE_NETWORK_SEARCH, "Force Network Search"},
    {NAS_NETWORK_REJECT, "Network Reject"},
    {NAS_GET_DRX, "Get DRX"},
    {NAS_GET_LTE_CPHY_CA_INFO, "Get LTE Cphy CA Info"},
    {NAS_SWI_GET_STATUS, "Swi Get Status"},
};

/* Indication register TLVs*/
enum {
  NAS_SVC_INDICATION_SYS_SELECT = 0x10,
  NAS_SVC_INDICATION_DDTM_EVENT = 0x12,
  NAS_SVC_INDICATION_SERVING_SYS = 0x13,
  NAS_SVC_INDICATION_DS_PREF = 0x14,
  NAS_SVC_INDICATION_SUBSCRIPTION_INFO = 0x15,
  NAS_SVC_INDICATION_NETWORK_TIME = 0x17,
  NAS_SVC_INDICATION_SYS_INFO = 0x18,
  NAS_SVC_INDICATION_SIGNAL_INFO = 0x19,
  NAS_SVC_INDICATION_ERROR_RATE = 0x1a,
  NAS_SVC_INDICATION_MANAGED_ROAMING = 0x1d,
  NAS_SVC_INDICATION_CURRENT_PLMN = 0x1e,
  NAS_SVC_INDICATION_EMBMS_STATE = 0x1f,
  NAS_SVC_INDICATION_RF_BAND_INFO = 0x20,
  NAS_SVC_INDICATION_NETWORK_REJECT_DATA = 0x21,
  NAS_SVC_INDICATION_OPERATOR_NAME = 0x22,
  NAS_SVC_INDICATION_PLMN_MODE_BIT = 0x23,
  NAS_SVC_INDICATION_RTRE_CONFIG = 0x24,
  NAS_SVC_INDICATION_IMS_PREFERENCE = 0x25,
  NAS_SVC_INDICATION_EMERGENCY_STATE_READY = 0x26,
  NAS_SVC_INDICATION_LTE_NETWORK_TIME = 0x27,
  NAS_SVC_INDICATION_LTE_CARRIER_AGG_INFO = 0x28,
  NAS_SVC_INDICATION_SUBSCRIPTION_CHANGE = 0x29,
  NAS_SVC_INDICATION_SERVICE_ACCESS_CLASS_BARRING = 0x2a,
  NAS_SVC_INDICATION_DATA_SUBSCRIPTION_PRIORITY_CHANGE = 0x2d,
  NAS_SVC_INDICATION_CALL_MODE_STATUS = 0x2f,
  NAS_SVC_INDICATION_EMERGENCY_MODE_STATUS = 0x33,
  NAS_SVC_INDICATION_GET_CELL_INFO = 0x34,
  NAS_SVC_INDICATION_EXTENDED_DISCONTINUOUS_RECEIVE_CHANGE = 0x35, // edrx
  NAS_SVC_INDICATION_LTE_RACH_FAILURE_INFO = 0x36,
  NAS_SVC_INDICATION_LTE_RRC_TX_INFO = 0x37,
  NAS_SVC_INDICATION_SUB_BLOCK_STATUS_INFO = 0x38,
  NAS_SVC_INDICATION_EMERGENCY_911_SEARCH_FAILURE_INFO = 0x39,
  NAS_SVC_INDICATION_ARFCN_LIST_INFO = 0x3b,
  NAS_SVC_INDICATION_GET_RF_AVAILABILITY = 0x3d,
};

/* Cell location info */

enum {
  NAS_CELL_LAC_INFO_CELL_ID = 0x10,
  NAS_CELL_LAC_INFO_UMTS_CELL_INFO = 0x11,
  NAS_CELL_LAC_INFO_CDMA_CELL_INFO = 0x12,
  NAS_CELL_LAC_INFO_LTE_INTRA_INFO = 0x13,
  NAS_CELL_LAC_INFO_LTE_INTER_INFO = 0x14,
  NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM = 0x15,
  NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA = 0x16,
  NAS_CELL_LAC_INFO_UMTS_CELL_ID = 0x17,            
  NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR = 0x18,
  NAS_CELL_LAC_INFO_CDMA_RX_INFO = 0x19,
  NAS_CELL_LAC_INFO_HDR_RX_INFO = 0x1a,
  NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED = 0x1b,  
  NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED = 0x1c,
  NAS_CELL_LAC_INFO_WCDMA_GSM_NEIGHBOUT_CELL_EXTENDED = 0x1d,
  NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV = 0x1e,
  NAS_CELL_LAC_INFO_WCDMA_INFO_ACTIVE_SET = 0x1f,
  NAS_CELL_LAC_INFO_WCDMA_ACTIVE_SET_REF_RADIO_LINK = 0x20,
  NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO = 0x21,
  NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO = 0x22, 
  NAS_CELL_LAC_INFO_WCDMA_EXTENDED_INFO_AS = 0x23,
  NAS_CELL_LAC_INFO_SCELL_GERAN_CONF = 0x24,              
  NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT = 0x25,           
  NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ = 0x26,        
  NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN = 0x27,
  NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN = 0x28,
  NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN = 0x29,
  NAS_CELL_LAC_INFO_NAS_INFO_EMM_STATE = 0x2a,
  NAS_CELL_LAC_INFO_NAS_RRC_STATE = 0x2c,
  NAS_CELL_LAC_INFO_LTE_INFO_RRC_STATE = 0x2d,
};
/* Inherited from cell.h*/
enum {
  NAS_SIGNAL_REPORT_TYPE_CDMA = 0x10,
  NAS_SIGNAL_REPORT_TYPE_CDMA_HDR = 0x11,
  NAS_SIGNAL_REPORT_TYPE_GSM = 0x12,
  NAS_SIGNAL_REPORT_TYPE_WCDMA = 0x13,
  NAS_SIGNAL_REPORT_TYPE_LTE = 0x14,
};
/* Cell LAC structures */

struct nmr_cell_data {
  uint32_t cell_id;
  uint8_t plmn[3];
  uint16_t lac;
  uint16_t arfcn;
  uint8_t bsic;
  uint16_t rx_level;
} __attribute__((packed));

struct umts_monitored_cells {
  uint16_t uarfcn;
  uint16_t psc;
  int16_t rscp;
  int16_t ecio;

} __attribute__((packed));

struct lte_cell_info {
  uint16_t phy_cell_id;
  int16_t rsrq;
  int16_t rsrp;
  int16_t rssi_level;
  int16_t srx_level;
} __attribute__((packed));

struct lte_inter_freq_instance {
  uint16_t earfcn;
  uint8_t srxlev_low_threshold;
  uint8_t srxlev_high_threshold;
  uint8_t reselect_priority;
  uint8_t num_cells;
  struct lte_cell_info lte_cell_info[0];
} __attribute__((packed));

struct lte_gsm_cell_neighbour {
  uint16_t arfcn;
  uint8_t band;
  uint8_t is_cell_id_valid;
  uint8_t bsic;
  int16_t rssi;
  int16_t srxlev;
} __attribute__((packed));

struct lte_gsm_neighbours {
  uint8_t cell_reselect_priority;
  uint8_t high_priority_reselect_threshold;
  uint8_t low_prio_reselect_threshold;
  uint8_t ncc_mask;
  uint8_t num_cells;
  struct lte_gsm_cell_neighbour lte_gsm_cell_neighbour[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_CELL_ID = 0x10,
struct cell_id { 
  uint8_t id;
  uint16_t len;
  uint32_t cell_id;
  uint8_t plmn[3];
  uint16_t lac;
  uint16_t arfcn;
  uint8_t bsic;
  uint32_t timing_advance;
  uint16_t rx_level;
  uint8_t nmr_instance; // num of elems below
  struct nmr_cell_data nmr_data[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_UMTS_CELL_INFO = 0x11,
struct nas_lac_umts_cell_info {
  uint8_t id;
  uint16_t len;
  uint16_t cell_id;
  uint8_t plmn[3];
  uint16_t lac;
  uint16_t uarfcn;
  uint16_t psc;
  int16_t rscp;
  int16_t ecio;
  uint8_t instances; // num of elems below?
  struct umts_monitored_cells monitored_cells[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_CDMA_CELL_INFO = 0x12,
struct nas_lac_cdma_cell_info { 
  uint8_t id;
  uint16_t len;
  uint16_t system_id;
  uint16_t network_id;
  uint16_t base_station_id;
  uint16_t reference_pn;
  uint32_t base_latitude;
  uint32_t base_longitude;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INTRA_INFO = 0x13,
struct nas_lac_lte_intra_cell_info {
  uint8_t id;
  uint16_t len;
  uint8_t is_idle;
  uint8_t plmn[3];
  uint16_t tracking_area_code;
  uint32_t cell_id;
  uint16_t earfcn;
  uint16_t serv_cell_id;
  uint8_t serving_freq_prio;
  uint8_t inter_frequency_search_threshold;
  uint8_t serving_cell_lower_threshold;
  uint8_t reselect_threshold;
  uint8_t num_of_cells;
  struct lte_cell_info lte_cell_info[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INTER_INFO = 0x14,
struct nas_lac_lte_inter_cell_info {
  uint8_t id;
  uint16_t len;
  uint8_t is_idle;
  uint8_t num_instances;
  struct lte_inter_freq_instance lte_inter_freq_instance[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_GSM 0x15
struct nas_lac_lte_gsm_neighbour_info { 
  uint8_t id;
  uint16_t len;
  uint8_t is_idle;
  uint8_t num_instances;
  struct lte_gsm_neighbours lte_gsm_neighbours[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INFO_NEIGHBOUR_WCDMA = 0x16,
struct wcdma_cells {
  uint16_t psc;
  int16_t cpich_rscp;
  int16_t cpich_ecno;
  int16_t srx_level;
} __attribute__((packed));

struct lte_wcdma_neighbours {
  uint16_t uarfcn;
  uint8_t cell_reselect_priority;
  uint16_t low_prio_reselect_threshold;
  uint16_t high_priority_reselect_threshold;
  uint8_t num_cells;
  struct wcdma_cells wcdma_cells[0];
} __attribute__((packed));

struct nas_lac_lte_wcdma_neighbour_info {
  uint8_t id;
  uint16_t len;
  uint8_t is_idle;
  uint8_t num_instances;
  struct lte_wcdma_neighbours lte_wcdma_neighbours[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_UMTS_CELL_ID = 0x17,
struct nas_lac_umts_cell_id {
  uint8_t id;
  uint16_t len;
  uint32_t cell_id;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_WCDMA_INFO_LTE_NEIGHBOUR = 0x18,
struct wcdma_lte_neighbour {
  uint16_t earfcn;
  uint16_t phy_cell_id;
  uint32_t rsrp;
  uint32_t rsrq;
  int16_t srx_level;
  uint8_t is_cell_tdd; // bool
} __attribute__((packed));

struct nas_lac_wcdma_lte_neighbour_info {
  uint8_t id;
  uint16_t len;
  uint32_t rrc_state;
  uint8_t num_cells;
  struct wcdma_lte_neighbour wcdma_lte_neighbour[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_GSM_CELL_INFO_EXTENDED = 0x1b,
struct nas_lac_extended_gsm_cell_info {
  uint8_t id;
  uint16_t len;
  uint16_t timing_advance;
  uint16_t bcch;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_WCDMA_CELL_INFO_EXTENDED = 0x1c,
struct nas_lac_extended_wcdma_cell_info {
  uint8_t id;
  uint16_t len;
  uint32_t wcdma_power_db;
  uint32_t wcdma_tx_power_db;
  uint16_t downlink_error_rate; // percent
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INFO_TIMING_ADV = 0x1e,
struct nas_lac_lte_timing_advance_info {
  uint8_t id;
  uint16_t len;
  int32_t timing_advance;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_EXTENDED_GERAN_INFO = 0x21,
struct nas_lac_extended_geran_info {
  uint8_t id;
  uint16_t len;
  uint32_t cell_id;
  uint8_t plmn[3];
  uint16_t lac;
  uint16_t arfcn;
  uint8_t bsic;
  uint32_t timing_advance;
  uint16_t rx_level;
  uint8_t num_instances;
  struct nmr_cell_data nmr_cell_data[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_UMTS_EXTENDED_INFO = 0x22,//
struct umts_monitored_cell_extended_info {
  uint16_t uarfcn;
  uint16_t psc;
  int16_t rscp;
  int16_t ecio;
  int16_t squal;
  int16_t srx_level;
  int16_t rank;
  uint8_t cell_set;
} __attribute__((packed));

struct nas_lac_umts_extended_info {
  uint8_t id;
  uint16_t len;
  uint16_t cell_id;
  uint8_t plmn[3];
  uint16_t lac;
  uint16_t uarfcn;
  uint16_t psc;
  int16_t rscp;
  int16_t ecio;
  int16_t squal;
  int16_t srx_level;
  uint8_t num_instances;
  struct umts_monitored_cell_extended_info umts_monitored_cell_extended_info[0];
} __attribute__((packed));

// NAS_CELL_LAC_INFO_SCELL_GERAN_CONF = 0x24,
struct nas_lac_scell_geran_config {
  uint8_t id;
  uint16_t len;
  uint8_t is_pbcch_present;
  uint8_t gprs_min_rx_level_access;
  uint8_t max_tx_power_cch;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_CURRENT_L1_TIMESLOT = 0x25,
struct nas_lac_current_l1_timeslot {
  uint8_t id;
  uint16_t len;
  uint8_t timeslot_num;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_DOPPLER_MEASUREMENT_HZ = 0x26,
struct nas_lac_doppler_measurement {
  uint8_t id;
  uint16_t len;
  uint16_t doppler_measurement_hz;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTRA_EARFCN = 0x27,
struct nas_lac_lte_extended_intra_earfcn_info {
  uint8_t id;
  uint16_t len;
  uint32_t earfcn;
} __attribute__((packed));

// NAS_CELL_LAC_INFO_LTE_INFO_EXTENDED_INTER_EARFCN = 0x28,
struct nas_lac_lte_extended_interfrequency_earfcn {
  uint8_t id;
  uint16_t len;
  uint32_t earfcn;

} __attribute__((packed));

//   NAS_CELL_LAC_INFO_WCDMA_INFO_EXTENDED_LTE_NEIGHBOUR_EARFCN = 0x29,
struct nas_lac_wcdma_extended_lte_neighbour_info_earfcn {
  uint8_t id;
  uint16_t len;
  uint8_t num_instances;
  uint32_t earfcn[0];
} __attribute__((packed));
/* Info structures */
struct carrier_name_string {
  uint8_t id; // 0x10 in msgid NAS_OPERATOR_NAME
  uint16_t len;
  uint16_t instance;
  uint8_t *operator_name[0];
} __attribute__((packed));

struct carrier_mcc_mnc {
  uint8_t id; // 0x11 in msgid NAS_OPERATOR_NAME
  uint16_t len;
  uint16_t isntance;
  uint8_t mcc[3];
  uint8_t mnc[2];
  uint16_t lac1;
  uint16_t lac2;
  uint8_t plmn_record_id;
  uint8_t instance2; //??
} __attribute__((packed));

/* INFO RETRIEVED FROM SERVING SYSTTEM */
struct service_capability { // 0x11, an array
  uint8_t gprs;             // 0x01
  uint8_t edge;             // 0x02
  uint8_t hsdpa;            // 0x03
  uint8_t hsupa;            // 0x04
  uint8_t wcdma;            // 0x05
  uint8_t gsm;              // 0x0a
  uint8_t lte;              // 0x0b
  uint8_t hsdpa_plus;       // 0x0c
  uint8_t dc_hsdpa_plus;    // 0x0d
};
struct nas_serving_system_state {
  uint8_t id;                  // 0x01 in msgid NAS_SERVING_SYSTEM
  uint16_t len;                // 6bytes
  uint8_t registration_status; // 00 not registed, 01 registered, 02 searchign,
                               // 03 forbidden, 04 unknown
  uint8_t cs_attached;         // 00 unknown, 01 attached, 02 detached
  uint8_t ps_attached;         // 00 unknown, 01 attached, 02 detached
  uint8_t radio_access;        // 00 unknown, 01 3gpp2, 02 3gpp
  uint8_t
      *connected_interfaces[0]; // 00 no interface (no service), 1 CDMA, 2 EVDO,
                                // 3 AMPS, 4 GSM, 5 UMTS, 6-7 unknown, 8 LTE
} __attribute__((packed));

/* Tracking data*/
/* 
 * What do we need:
 |-> PLMN (Public Land Mobile Network ID)
 |-> Type of service (GSM - WCDMA - LTE)
 |-> Cell Identifier
 |---> Physical Cell ID
 |---> Current Cell ID
 |---> BSIC (Base station Identity Code) (GSM)
 |---> BCCH (GSM)
 |---> PSC (Primary Scrambing Code) (UMTS)
 |-> Location for that cell ID
 |---> Current Location Area Code (GSM/WCDMA)
 |---> Current Tracking Area Code (LTE)
 |-> Channel
 |---> ARFCN (Absolute Radio Frequency Channel Number) (GSM/WCDMA)
 |---> EARFCN (E-UTRA Absolute Frequency Channel Number) (LTE)
 |-> TX / RX Levels
 |---> Min SRX Level
 |---> Max SRX Level
 |---> Avg. SRX Level
 |-> Min TX Power (dB)(UMTS)
 |-> Max TX Power (dB)(UMTS)
 |-> Avg. TX Power (dB)(UMTS)
 |---> Squal (LTE only)
 |---> Timestamp of last indication for this cell
*/

/* OpenCellid */
enum ocid_radio_type {
  OCID_RADIO_GSM = 0x01,
  OCID_RADIO_CDMA,
  OCID_RADIO_UMTS = 0x05,
  OCID_RADIO_LTE = 0x0b,
  OCID_RADIO_NR = 0x0e,
} __attribute__((__packed__));

struct ocid_cell_slim {
  uint8_t radio;
  uint32_t area;
  uint32_t cell;
  float lon;
  float lat;
  uint32_t range;
  uint32_t updated;
  int16_t average_signal;
} __attribute__ ((__packed__));

struct nas_report {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t type_of_service; // GSM...LTE

  uint16_t lac;

  uint16_t phy_cell_id;
  uint32_t cell_id;

  uint8_t bsic;
  uint16_t bcch;
  uint16_t psc;

  uint16_t arfcn;

  int16_t srx_level_min;
  int16_t srx_level_max;
  int16_t srx_level_avg;

  uint16_t rx_level_min;
  uint16_t rx_level_max;
  uint16_t rx_level_avg;

  uint8_t found_in_network;
  uint8_t opencellid_verified;
};

struct basic_network_status {
  uint8_t operator_name[32];
  uint8_t mcc[4];
  uint8_t mnc[3];
  uint16_t location_area_code_1;
  uint16_t location_area_code_2;
  struct service_capability service_capability;
  uint8_t network_registration_status;
  uint8_t circuit_switch_attached;
  uint8_t packet_switch_attached;
  uint8_t radio_access;
  uint8_t network_type; // LTE / WCDMA / GSM / ??
  uint8_t signal_level; // in dB
};


struct network_status_reports {
  uint8_t in_use;
  struct nas_report report;
};

/* Functions */
uint8_t is_cellid_data_missing();
void set_cellid_data_missing_as_requested();
uint8_t *get_current_mcc();
uint8_t *get_current_mnc();
uint8_t get_network_type();
struct nas_report get_current_cell_report();
struct basic_network_status get_network_status();
uint8_t get_signal_strength();
uint8_t nas_is_network_in_service();
const char *get_nas_command(uint16_t msgid);

int nas_request_cell_location_info();
int nas_request_signal_info();

void *register_to_nas_service();
int handle_incoming_nas_message(uint8_t *buf, size_t buf_len);
#endif
