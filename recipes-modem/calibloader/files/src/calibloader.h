#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_PATH "/etc/acdb"
#define ION_DEV_NODE "/dev/ion"
#define DEV_NODE "/dev/msm_audio_cal"
#define ACDB_BLOCK_SIZE 4096
#define ACDB_TOTAL_IDS 4
#define MAX_BUF_SIZE 16384 // 4*4096
#define BUF_ALIGN 0x1000

#define ION_IOC_MAGIC 'I'
#define ION_IOC_ALLOC _IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
typedef int ion_user_handle_t;
struct ion_allocation_data {
  size_t len;
  size_t align;
  unsigned int heap_mask;
  unsigned int flags;
  ion_user_handle_t handle;
};

struct acdb_file_entry {
  char filename[32];
  int filetype;
};
struct calibration_block {
  char block_name[8];
  uint32_t block_data;
};

#define ACDB_FILE_MAGIC 0x4244444e534d4351U
#define ACDB_MAGIC_ID_SIZE 8 // 8 char
#define ACDB_MAGIC_TYPE_SIZE 4 // 4 char

#define ACDB_MAGIC_TYPE_AVDB 0x42445641 // "AVDB"
#define ACDB_MAGIC_TYPE_GCDB 0x42444347 // "GCDB"

#define ACDB_REAL_DATA_OFFSET 638 // All acdb data seem to start here
#define ACDB_CALIB_BLOCK_NAME                                                  \
  8 // All acdbs are aligned in "blocks", all blocks seem to be 8 byte long
#define ACDB_CALIB_BLOCK_SIZE 4 // This is wrong, they change depending on block
struct acdbfile_header {
  uint64_t magic;     // Should be ACDB_FILE_MAGIC
  uint64_t dummy1;    // Always 0x00
  uint32_t file_type; // ACDB_MAGIC_TYPE_AVDB || ACDB_MAGIC_TYPE_GCDB
  uint32_t dummy2;    // No fucking clue, changes between files
  uint32_t total_size; //
  uint32_t data_size;
};

// From the files themselves
static const struct {
  const char *block_name;
} block_names[] = {
    "AANCCDFT", "AANCCDOT", "AANCLUT0", "ACGDCDFT", "ACGDCDOT", "ACGDLUT0",
    "ACGICDFT", "ACGICDOT", "ACGILUT0", "ACSWVERS", "ADSPVERS", "ADSTCDFT",
    "ADSTCDOT", "ADSTLUT0", "AFECCDFT", "AFECCDOT", "AFECLUT0", "ASTMCDFT",
    "ASTMCDOT", "ASTMLUT0", "AVOLCDFT", "AVOLCDOT", "AVOLLUT0", "AVP3CDFT",
    "AVP3CDOT", "AVP3LUT0", "CDFSLUT0", "DATAPOOL", "DATAPOOL", "DEVCATIN",
    "DPROPLUT", "EVP3CDFT", "EVP3CDOT", "EVP3LUT0", "GLBLLUT",  "GPROPLUT",
    "LSMCCDFT", "LSMCCDOT", "LSMCLUT0", "MINFOLUT", "VCD2CDFT", "VCD2CDOT",
    "VCGDCDFT", "VCGDCDOT", "VCGDLUT0", "VCGICDFT", "VCGICDOT", "VCGILUT0",
    "VDPICDFT", "VDPICDOT", "VDPILUT0", "VGD2LUT0", "VPDYCDFT", "VPDYCDOT",
    "VPDYCVD0", "VPDYLUT0", "VPDYOFST", "VPSTCDFT", "VPSTCDOT", "VPSTCVD0",
    "VPSTLUT0", "VPSTOFST", "VST2CDFT", "VST2CDOT", "VST2LUT0", "VSTMCDFT",
    "VSTMCDOT", "VSTMLUT0", "VVP3CDFT", "VVP3CDOT", "VVP3LUT0"};

// from q6voice.h
#define CVD_VERSION_0_0 "0.0"
#define CVD_INT_VERSION_0_0 1

// From uapi -> msm_audio_calibration.h
#define CAL_IOCTL_MAGIC 'a'

#define AUDIO_ALLOCATE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 200, void *)
#define AUDIO_DEALLOCATE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 201, void *)
#define AUDIO_PREPARE_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 202, void *)
#define AUDIO_SET_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 203, void *)
#define AUDIO_GET_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 204, void *)
#define AUDIO_POST_CALIBRATION _IOWR(CAL_IOCTL_MAGIC, 205, void *)

/* For Real-Time Audio Calibration */
#define AUDIO_GET_RTAC_ADM_INFO _IOR(CAL_IOCTL_MAGIC, 207, void *)
#define AUDIO_GET_RTAC_VOICE_INFO _IOR(CAL_IOCTL_MAGIC, 208, void *)
#define AUDIO_GET_RTAC_ADM_CAL _IOWR(CAL_IOCTL_MAGIC, 209, void *)
#define AUDIO_SET_RTAC_ADM_CAL _IOWR(CAL_IOCTL_MAGIC, 210, void *)
#define AUDIO_GET_RTAC_ASM_CAL _IOWR(CAL_IOCTL_MAGIC, 211, void *)
#define AUDIO_SET_RTAC_ASM_CAL _IOWR(CAL_IOCTL_MAGIC, 212, void *)
#define AUDIO_GET_RTAC_CVS_CAL _IOWR(CAL_IOCTL_MAGIC, 213, void *)
#define AUDIO_SET_RTAC_CVS_CAL _IOWR(CAL_IOCTL_MAGIC, 214, void *)
#define AUDIO_GET_RTAC_CVP_CAL _IOWR(CAL_IOCTL_MAGIC, 215, void *)
#define AUDIO_SET_RTAC_CVP_CAL _IOWR(CAL_IOCTL_MAGIC, 216, void *)
#define AUDIO_GET_RTAC_AFE_CAL _IOWR(CAL_IOCTL_MAGIC, 217, void *)
#define AUDIO_SET_RTAC_AFE_CAL _IOWR(CAL_IOCTL_MAGIC, 218, void *)
enum {
  CVP_VOC_RX_TOPOLOGY_CAL_TYPE = 0,
  CVP_VOC_TX_TOPOLOGY_CAL_TYPE,
  CVP_VOCPROC_STATIC_CAL_TYPE,
  CVP_VOCPROC_DYNAMIC_CAL_TYPE,
  CVS_VOCSTRM_STATIC_CAL_TYPE,
  CVP_VOCDEV_CFG_CAL_TYPE,
  CVP_VOCPROC_STATIC_COL_CAL_TYPE,
  CVP_VOCPROC_DYNAMIC_COL_CAL_TYPE,
  CVS_VOCSTRM_STATIC_COL_CAL_TYPE,

  ADM_TOPOLOGY_CAL_TYPE,
  ADM_CUST_TOPOLOGY_CAL_TYPE,
  ADM_AUDPROC_CAL_TYPE,
  ADM_AUDVOL_CAL_TYPE,

  ASM_TOPOLOGY_CAL_TYPE,
  ASM_CUST_TOPOLOGY_CAL_TYPE,
  ASM_AUDSTRM_CAL_TYPE,

  AFE_COMMON_RX_CAL_TYPE,
  AFE_COMMON_TX_CAL_TYPE,
  AFE_ANC_CAL_TYPE,
  AFE_AANC_CAL_TYPE,
  AFE_FB_SPKR_PROT_CAL_TYPE,
  AFE_HW_DELAY_CAL_TYPE,
  AFE_SIDETONE_CAL_TYPE,
  AFE_TOPOLOGY_CAL_TYPE,
  AFE_CUST_TOPOLOGY_CAL_TYPE,

  LSM_CUST_TOPOLOGY_CAL_TYPE,
  LSM_TOPOLOGY_CAL_TYPE,
  LSM_CAL_TYPE,

  ADM_RTAC_INFO_CAL_TYPE,
  VOICE_RTAC_INFO_CAL_TYPE,
  ADM_RTAC_APR_CAL_TYPE,
  ASM_RTAC_APR_CAL_TYPE,
  VOICE_RTAC_APR_CAL_TYPE,

  MAD_CAL_TYPE,
  ULP_AFE_CAL_TYPE,
  ULP_LSM_CAL_TYPE,

  DTS_EAGLE_CAL_TYPE,
  AUDIO_CORE_METAINFO_CAL_TYPE,
  SRS_TRUMEDIA_CAL_TYPE,

  CORE_CUSTOM_TOPOLOGIES_CAL_TYPE,
  ADM_RTAC_AUDVOL_CAL_TYPE,

  ULP_LSM_TOPOLOGY_ID_CAL_TYPE,
  AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE,
  AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE,
  MAX_CAL_TYPES,
};

#define AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE
#define AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE

enum {
  VERSION_0_0,
};

enum {
  PER_VOCODER_CAL_BIT_MASK = 0x10000,
};

#define MAX_IOCTL_CMD_SIZE 512

/* common structures */

struct audio_cal_header {
  int32_t data_size;
  int32_t version;
  int32_t cal_type;
  int32_t cal_type_size;
};

struct audio_cal_type_header {
  int32_t version;
  int32_t buffer_number;
};

struct audio_cal_data {
  /* Size of cal data at mem_handle allocation or at vaddr */
  int32_t cal_size;
  /* If mem_handle if shared memory is used*/
  int32_t mem_handle;
  /* size of virtual memory if shared memory not used */
};

/* AUDIO_ALLOCATE_CALIBRATION */
struct audio_cal_type_alloc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};

struct audio_cal_alloc {
  struct audio_cal_header hdr;
  struct audio_cal_type_alloc cal_type;
};

/* AUDIO_DEALLOCATE_CALIBRATION */
struct audio_cal_type_dealloc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};

struct audio_cal_dealloc {
  struct audio_cal_header hdr;
  struct audio_cal_type_dealloc cal_type;
};

/* AUDIO_PREPARE_CALIBRATION */
struct audio_cal_type_prepare {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};

struct audio_cal_prepare {
  struct audio_cal_header hdr;
  struct audio_cal_type_prepare cal_type;
};

/* AUDIO_POST_CALIBRATION */
struct audio_cal_type_post {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};

struct audio_cal_post {
  struct audio_cal_header hdr;
  struct audio_cal_type_post cal_type;
};

/*AUDIO_CORE_META_INFO */

struct audio_cal_info_metainfo {
  uint32_t nKey;
};

/* Cal info types */
enum { RX_DEVICE, TX_DEVICE, MAX_PATH_TYPE };

struct audio_cal_info_adm_top {
  int32_t topology;
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t app_type;
  int32_t sample_rate;
};

struct audio_cal_info_audproc {
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t app_type;
  int32_t sample_rate;
};

struct audio_cal_info_audvol {
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t app_type;
  int32_t vol_index;
};

struct audio_cal_info_afe {
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t sample_rate;
};

struct audio_cal_info_afe_top {
  int32_t topology;
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t sample_rate;
};

struct audio_cal_info_asm_top {
  int32_t topology;
  int32_t app_type;
};

struct audio_cal_info_audstrm {
  int32_t app_type;
};

struct audio_cal_info_aanc {
  int32_t acdb_id;
};

#define MAX_HW_DELAY_ENTRIES 25

struct audio_cal_hw_delay_entry {
  uint32_t sample_rate;
  uint32_t delay_usec;
};

struct audio_cal_hw_delay_data {
  uint32_t num_entries;
  struct audio_cal_hw_delay_entry entry[MAX_HW_DELAY_ENTRIES];
};

struct audio_cal_info_hw_delay {
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t property_type;
  struct audio_cal_hw_delay_data data;
};

enum msm_spkr_prot_states {
  MSM_SPKR_PROT_CALIBRATED,
  MSM_SPKR_PROT_CALIBRATION_IN_PROGRESS,
  MSM_SPKR_PROT_DISABLED,
  MSM_SPKR_PROT_NOT_CALIBRATED,
  MSM_SPKR_PROT_PRE_CALIBRATED,
  MSM_SPKR_PROT_IN_FTM_MODE
};
#define MSM_SPKR_PROT_IN_FTM_MODE MSM_SPKR_PROT_IN_FTM_MODE

enum msm_spkr_count { SP_V2_SPKR_1, SP_V2_SPKR_2, SP_V2_NUM_MAX_SPKRS };

struct audio_cal_info_spk_prot_cfg {
  int32_t r0[SP_V2_NUM_MAX_SPKRS];
  int32_t t0[SP_V2_NUM_MAX_SPKRS];
  uint32_t quick_calib_flag;
  uint32_t mode;
  /*
   * 0 - Start spk prot
   * 1 - Start calib
   * 2 - Disable spk prot
   */
};

struct audio_cal_info_sp_th_vi_ftm_cfg {
  uint32_t wait_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t ftm_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t mode;
  /*
   * 0 - normal running mode
   * 1 - Calibration
   * 2 - FTM mode
   */
};

struct audio_cal_info_sp_ex_vi_ftm_cfg {
  uint32_t wait_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t ftm_time[SP_V2_NUM_MAX_SPKRS];
  uint32_t mode;
  /*
   * 0 - normal running mode
   * 2 - FTM mode
   */
};

struct audio_cal_info_sp_ex_vi_param {
  int32_t freq_q20[SP_V2_NUM_MAX_SPKRS];
  int32_t resis_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t qmct_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t status[SP_V2_NUM_MAX_SPKRS];
};

struct audio_cal_info_sp_th_vi_param {
  int32_t r_dc_q24[SP_V2_NUM_MAX_SPKRS];
  int32_t temp_q22[SP_V2_NUM_MAX_SPKRS];
  int32_t status[SP_V2_NUM_MAX_SPKRS];
};

struct audio_cal_info_msm_spk_prot_status {
  int32_t r0[SP_V2_NUM_MAX_SPKRS];
  int32_t status;
};

struct audio_cal_info_sidetone {
  uint16_t enable;
  uint16_t gain;
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  int32_t mid;
  int32_t pid;
};

struct audio_cal_info_lsm_top {
  int32_t topology;
  int32_t acdb_id;
  int32_t app_type;
};

struct audio_cal_info_lsm {
  int32_t acdb_id;
  /* RX_DEVICE or TX_DEVICE */
  int32_t path;
  int32_t app_type;
};

struct audio_cal_info_voc_top {
  int32_t topology;
  int32_t acdb_id;
};

struct audio_cal_info_vocproc {
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  int32_t tx_sample_rate;
  int32_t rx_sample_rate;
};

enum {
  DEFAULT_FEATURE_SET,
  VOL_BOOST_FEATURE_SET,
};

struct audio_cal_info_vocvol {
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  /* DEFUALT_ or VOL_BOOST_FEATURE_SET */
  int32_t feature_set;
};

struct audio_cal_info_vocdev_cfg {
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
};

#define MAX_VOICE_COLUMNS 20

union audio_cal_col_na {
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
} __attribute__((packed));

struct audio_cal_col {
  uint32_t id;
  uint32_t type;
  union audio_cal_col_na na_value;
} __attribute__((packed));

struct audio_cal_col_data {
  uint32_t num_columns;
  struct audio_cal_col column[MAX_VOICE_COLUMNS];
} __attribute__((packed));

struct audio_cal_info_voc_col {
  int32_t table_id;
  int32_t tx_acdb_id;
  int32_t rx_acdb_id;
  struct audio_cal_col_data data;
};

/* AUDIO_SET_CALIBRATION & */
struct audio_cal_type_basic {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
};

struct audio_cal_basic {
  struct audio_cal_header hdr;
  struct audio_cal_type_basic cal_type;
};

struct audio_cal_type_adm_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_adm_top cal_info;
};

struct audio_cal_adm_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_adm_top cal_type;
};

struct audio_cal_type_metainfo {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_metainfo cal_info;
};

struct audio_core_metainfo {
  struct audio_cal_header hdr;
  struct audio_cal_type_metainfo cal_type;
};

struct audio_cal_type_audproc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_audproc cal_info;
};

struct audio_cal_audproc {
  struct audio_cal_header hdr;
  struct audio_cal_type_audproc cal_type;
};

struct audio_cal_type_audvol {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_audvol cal_info;
};

struct audio_cal_audvol {
  struct audio_cal_header hdr;
  struct audio_cal_type_audvol cal_type;
};

struct audio_cal_type_asm_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_asm_top cal_info;
};

struct audio_cal_asm_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_asm_top cal_type;
};

struct audio_cal_type_audstrm {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_audstrm cal_info;
};

struct audio_cal_audstrm {
  struct audio_cal_header hdr;
  struct audio_cal_type_audstrm cal_type;
};

struct audio_cal_type_afe {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_afe cal_info;
};

struct audio_cal_afe {
  struct audio_cal_header hdr;
  struct audio_cal_type_afe cal_type;
};

struct audio_cal_type_afe_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_afe_top cal_info;
};

struct audio_cal_afe_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_afe_top cal_type;
};

struct audio_cal_type_aanc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_aanc cal_info;
};

struct audio_cal_aanc {
  struct audio_cal_header hdr;
  struct audio_cal_type_aanc cal_type;
};

struct audio_cal_type_fb_spk_prot_cfg {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_spk_prot_cfg cal_info;
};

struct audio_cal_fb_spk_prot_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_fb_spk_prot_cfg cal_type;
};

struct audio_cal_type_sp_th_vi_ftm_cfg {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_th_vi_ftm_cfg cal_info;
};

struct audio_cal_sp_th_vi_ftm_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_th_vi_ftm_cfg cal_type;
};

struct audio_cal_type_sp_ex_vi_ftm_cfg {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_ex_vi_ftm_cfg cal_info;
};

struct audio_cal_sp_ex_vi_ftm_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_ex_vi_ftm_cfg cal_type;
};
struct audio_cal_type_hw_delay {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_hw_delay cal_info;
};

struct audio_cal_hw_delay {
  struct audio_cal_header hdr;
  struct audio_cal_type_hw_delay cal_type;
};

struct audio_cal_type_sidetone {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sidetone cal_info;
};

struct audio_cal_sidetone {
  struct audio_cal_header hdr;
  struct audio_cal_type_sidetone cal_type;
};

struct audio_cal_type_lsm_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_lsm_top cal_info;
};

struct audio_cal_lsm_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_lsm_top cal_type;
};

struct audio_cal_type_lsm {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_lsm cal_info;
};

struct audio_cal_lsm {
  struct audio_cal_header hdr;
  struct audio_cal_type_lsm cal_type;
};

struct audio_cal_type_voc_top {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_voc_top cal_info;
};

struct audio_cal_voc_top {
  struct audio_cal_header hdr;
  struct audio_cal_type_voc_top cal_type;
};

struct audio_cal_type_vocproc {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_vocproc cal_info;
};

struct audio_cal_vocproc {
  struct audio_cal_header hdr;
  struct audio_cal_type_vocproc cal_type;
};

struct audio_cal_type_vocvol {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_vocvol cal_info;
};

struct audio_cal_vocvol {
  struct audio_cal_header hdr;
  struct audio_cal_type_vocvol cal_type;
};

struct audio_cal_type_vocdev_cfg {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_vocdev_cfg cal_info;
};

struct audio_cal_vocdev_cfg {
  struct audio_cal_header hdr;
  struct audio_cal_type_vocdev_cfg cal_type;
};

struct audio_cal_type_voc_col {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_voc_col cal_info;
};

struct audio_cal_voc_col {
  struct audio_cal_header hdr;
  struct audio_cal_type_voc_col cal_type;
};

/* AUDIO_GET_CALIBRATION */
struct audio_cal_type_fb_spk_prot_status {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_msm_spk_prot_status cal_info;
};

struct audio_cal_fb_spk_prot_status {
  struct audio_cal_header hdr;
  struct audio_cal_type_fb_spk_prot_status cal_type;
};

struct audio_cal_type_sp_th_vi_param {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_th_vi_param cal_info;
};

struct audio_cal_sp_th_vi_param {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_th_vi_param cal_type;
};
struct audio_cal_type_sp_ex_vi_param {
  struct audio_cal_type_header cal_hdr;
  struct audio_cal_data cal_data;
  struct audio_cal_info_sp_ex_vi_param cal_info;
};

struct audio_cal_sp_ex_vi_param {
  struct audio_cal_header hdr;
  struct audio_cal_type_sp_ex_vi_param cal_type;
};
