// SPDX-License-Identifier: MIT
#ifndef __DMS_H__
#define __DMS_H__

#include "../inc/openqti.h"
#include "../inc/qmi.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Device Management Service
 *
 *
 *
 */

/* DMS Testing */

enum {
  DMS_RESET = 0x0000,
  DMS_SET_EVENT_REPORT = 0x0001,
  DMS_EVENT_REPORT = 0x0001,
  DMS_REGISTER_INDICATIONS = 0x0003,
  DMS_GET_CAPABILITIES = 0x0020,
  DMS_GET_MANUFACTURER = 0x0021,
  DMS_GET_MODEL = 0x0022,
  DMS_GET_REVISION = 0x0023,
  DMS_GET_MSISDN = 0x0024,
  DMS_GET_IDS = 0x0025,
  DMS_GET_POWER_STATE = 0x0026,
  DMS_UIM_SET_PIN_PROTECTION = 0x0027,
  DMS_UIM_VERIFY_PIN = 0x0028,
  DMS_UIM_UNBLOCK_PIN = 0x0029,
  DMS_UIM_CHANGE_PIN = 0x002A,
  DMS_UIM_GET_PIN_STATUS = 0x002B,
  DMS_GET_HARDWARE_REVISION = 0x002C,
  DMS_GET_OPERATING_MODE = 0x002D,
  DMS_SET_OPERATING_MODE = 0x002E,
  DMS_GET_TIME = 0x002F,
  DMS_GET_PRL_VERSION = 0x0030,
  DMS_GET_ACTIVATION_STATE = 0x0031,
  DMS_ACTIVATE_AUTOMATIC = 0x0032,
  DMS_ACTIVATE_MANUAL = 0x0033,
  DMS_GET_USER_LOCK_STATE = 0x0034,
  DMS_SET_USER_LOCK_STATE = 0x0035,
  DMS_SET_USER_LOCK_CODE = 0x0036,
  DMS_READ_USER_DATA = 0x0037,
  DMS_WRITE_USER_DATA = 0x0038,
  DMS_READ_ERI_FILE = 0x0039,
  DMS_RESTORE_FACTORY_DEFAULTS = 0x003A,
  DMS_VALIDATE_SERVICE_PROGRAMMING_CODE = 0x003B,
  DMS_UIM_GET_ICCID = 0x003C,
  DMS_SET_FIRMWARE_ID = 0x003E,
  DMS_UIM_GET_CK_STATUS = 0x0040,
  DMS_UIM_SET_CK_PROTECTION = 0x0041,
  DMS_UIM_UNBLOCK_CK = 0x0042,
  DMS_UIM_GET_IMSI = 0x0043,
  DMS_UIM_GET_STATE = 0x0044,
  DMS_GET_BAND_CAPABILITIES = 0x0045,
  DMS_GET_FACTORY_SKU = 0x0046,
  DMS_GET_FIRMWARE_PREFERENCE = 0x0047,
  DMS_SET_FIRMWARE_PREFERENCE = 0x0048,
  DMS_LIST_STORED_IMAGES = 0x0049,
  DMS_DELETE_STORED_IMAGE = 0x004A,
  DMS_SET_TIME = 0x004B,
  DMS_GET_STORED_IMAGE_INFO = 0x004C,
  DMS_GET_ALT_NET_CONFIG = 0x004D,
  DMS_SET_ALT_NET_CONFIG = 0x004E,
  DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE = 0x004F,
  DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE = 0x0050,
  DMS_GET_SOFTWARE_VERSION = 0x0051,
  DMS_SET_SERVICE_PROGRAMMING_CODE = 0x0052,
  DMS_GET_MAC_ADDRESS = 0x005C,
  DMS_GET_SUPPORTED_MESSAGES = 0x001E,
  
  DMS_GET_PSM_CONFIG = 0x0060,
  DMS_ENTER_PSM = 0x0061, 
  DMS_GET_PSM_INDICATION = 0x0062,
  DMS_GET_PSM_CONFIG_CHANGE_INDICATION = 0x0067,
  DMS_GET_IMS_SUBSCRIPTION_STATUS = 0x0068,

  DMS_HP_CHANGE_DEVICE_MODE = 0x5556,
  DMS_SWI_GET_CURRENT_FIRMWARE = 0x5556,
  DMS_SWI_GET_USB_COMPOSITION = 0x555B,
  DMS_SWI_SET_USB_COMPOSITION = 0x555C,
  DMS_FOXCONN_GET_FIRMWARE_VERSION = 0x555E,
  DMS_SET_FCC_AUTHENTICATION = 0x555F,
  DMS_FOXCONN_CHANGE_DEVICE_MODE = 0x5562,
  DMS_FOXCONN_SET_FCC_AUTHENTICATION = 0x5571,
  DMS_FOXCONN_SET_FCC_AUTHENTICATION_V2 = 0x5571,
};

static const struct {
  uint16_t id;
  const char *cmd;
} dms_svc_commands[] = {
    {DMS_RESET, "Reset"},
    {DMS_SET_EVENT_REPORT, "Set Event Report"},
    {DMS_EVENT_REPORT, "Event Report"},
    {DMS_REGISTER_INDICATIONS, "Register to DMS indications"},
    {DMS_GET_CAPABILITIES, "Get Capabilities"},
    {DMS_GET_MANUFACTURER, "Get Manufacturer"},
    {DMS_GET_MODEL, "Get Model"},
    {DMS_GET_REVISION, "Get Revision"},
    {DMS_GET_MSISDN, "Get MSISDN"},
    {DMS_GET_IDS, "Get IDs"},
    {DMS_GET_POWER_STATE, "Get Power State"},
    {DMS_UIM_SET_PIN_PROTECTION, "UIM Set PIN Protection"},
    {DMS_UIM_VERIFY_PIN, "UIM Verify PIN"},
    {DMS_UIM_UNBLOCK_PIN, "UIM Unblock PIN"},
    {DMS_UIM_CHANGE_PIN, "UIM Change PIN"},
    {DMS_UIM_GET_PIN_STATUS, "UIM Get PIN Status"},
    {DMS_GET_HARDWARE_REVISION, "Get Hardware Revision"},
    {DMS_GET_OPERATING_MODE, "Get Operating Mode"},
    {DMS_SET_OPERATING_MODE, "Set Operating Mode"},
    {DMS_GET_TIME, "Get Time"},
    {DMS_GET_PRL_VERSION, "Get PRL Version"},
    {DMS_GET_ACTIVATION_STATE, "Get Activation State"},
    {DMS_ACTIVATE_AUTOMATIC, "Activate Automatic"},
    {DMS_ACTIVATE_MANUAL, "Activate Manual"},
    {DMS_GET_USER_LOCK_STATE, "Get User Lock State"},
    {DMS_SET_USER_LOCK_STATE, "Set User Lock State"},
    {DMS_SET_USER_LOCK_CODE, "Set User Lock Code"},
    {DMS_READ_USER_DATA, "Read User Data"},
    {DMS_WRITE_USER_DATA, "Write User Data"},
    {DMS_READ_ERI_FILE, "Read ERI File"},
    {DMS_RESTORE_FACTORY_DEFAULTS, "Restore Factory Defaults"},
    {DMS_VALIDATE_SERVICE_PROGRAMMING_CODE,
     "Validate Service Programming Code"},
    {DMS_UIM_GET_ICCID, "UIM Get ICCID"},
    {DMS_SET_FIRMWARE_ID, "Set Firmware ID"},
    {DMS_UIM_GET_CK_STATUS, "UIM Get CK Status"},
    {DMS_UIM_SET_CK_PROTECTION, "UIM Set CK Protection"},
    {DMS_UIM_UNBLOCK_CK, "UIM Unblock CK"},
    {DMS_UIM_GET_IMSI, "UIM Get IMSI"},
    {DMS_UIM_GET_STATE, "UIM Get State"},
    {DMS_GET_BAND_CAPABILITIES, "Get Band Capabilities"},
    {DMS_GET_FACTORY_SKU, "Get Factory SKU"},
    {DMS_GET_FIRMWARE_PREFERENCE, "Get Firmware Preference"},
    {DMS_SET_FIRMWARE_PREFERENCE, "Set Firmware Preference"},
    {DMS_LIST_STORED_IMAGES, "List Stored Images"},
    {DMS_DELETE_STORED_IMAGE, "Delete Stored Image"},
    {DMS_SET_TIME, "Set Time"},
    {DMS_GET_STORED_IMAGE_INFO, "Get Stored Image Info"},
    {DMS_GET_ALT_NET_CONFIG, "Get Alt Net Config"},
    {DMS_SET_ALT_NET_CONFIG, "Set Alt Net Config"},
    {DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE, "Get Boot Image Download Mode"},
    {DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE, "Set Boot Image Download Mode"},
    {DMS_GET_SOFTWARE_VERSION, "Get Software Version"},
    {DMS_SET_SERVICE_PROGRAMMING_CODE, "Set Service Programming Code"},
    {DMS_GET_MAC_ADDRESS, "Get MAC Address"},
    {DMS_GET_SUPPORTED_MESSAGES, "Get Supported Messages"},
    {DMS_GET_PSM_CONFIG, "Get PSM Config"},
    {DMS_ENTER_PSM, "Enter Power Save Mode"},
    {DMS_GET_PSM_INDICATION, "Power Save Mode Indication"},
    {DMS_GET_PSM_CONFIG_CHANGE_INDICATION, "Power Save Mode configuration change"},
    {DMS_GET_IMS_SUBSCRIPTION_STATUS, "IMS Subscription state change indication"},
    {DMS_HP_CHANGE_DEVICE_MODE, "HP Change Device Mode"},
    {DMS_SWI_GET_CURRENT_FIRMWARE, "Swi Get Current Firmware"},
    {DMS_SWI_GET_USB_COMPOSITION, "Swi Get USB Composition"},
    {DMS_SWI_SET_USB_COMPOSITION, "Swi Set USB Composition"},
    {DMS_FOXCONN_GET_FIRMWARE_VERSION, "Foxconn Get Firmware Version"},
    {DMS_SET_FCC_AUTHENTICATION, "Set FCC Authentication"},
    {DMS_FOXCONN_CHANGE_DEVICE_MODE, "Foxconn Change Device Mode"},
    {DMS_FOXCONN_SET_FCC_AUTHENTICATION, "Foxconn Set FCC Authentication"},
    {DMS_FOXCONN_SET_FCC_AUTHENTICATION_V2,
     "Foxconn Set FCC Authentication v2"},
};

/* Firmware version parsing */

enum {
    FIRMWARE_STR_UNKNOWN = 0x0000,
    FIRMWARE_STR_SBL = 0x000001,
    FIRMWARE_STR_TZ = 0x000002,
    FIRMWARE_STR_TZ_SEC = 0x000003,
    FIRMWARE_STR_RPM = 0x000004,
    FIRMWARE_STR_BOOTLOADER = 0x000007,
    FIRMWARE_STR_APPS = 0x000008,
    FIRMWARE_STR_MPSS = 0x000009,
    FIRMWARE_STR_ADSP = 0x00000a,
};

struct dms_firmware_rel_string {
    uint32_t section;
    uint8_t version_length;
    uint8_t *fw_version[0];
} __attribute__((packed));

struct dms_fw_version_strings_info {
    uint8_t id;
    uint16_t len;
    uint8_t version_count;
 //   struct dms_firmware_rel_string *dms_firmware_rel_string[0];
} __attribute__((packed));

/* Event Reporting stuff */
enum { // all use uint8_t's
    EVENT_TLV_POWER_STATE = 0x10,
    EVENT_TLV_PIN_STATE = 0x12,
    EVENT_ACTIVATION_STATE = 0x13,
    EVENT_OP_MODE = 0x14,
    EVENT_UIM_STATE = 0x15,
    EVENT_AIRPLANE_MODE_STATE = 0x16,
};

enum {
    DMS_EVENT_POWER_STATE = 0x10, //u16
    DMS_EVENT_PIN_STATUS = 0x11,
    DMS_EVENT_PIN2_STATUS = 0x12,
    DMS_EVENT_ACT_STATE = 0x13, //u16: 0(not) || 1(act) || 2(connecting) || 3(connected) || 4-10(ota service provisioning state)
    DMS_EVENT_OPERATING_MODE = 0x14, // u8 0-8 (online, low power, test mode, offline, reset, shutdown, persistlpm, lpm_only, gsm_testmode)
    DMS_EVENT_CAPABILITY = 0x1c, 
};


enum { // all use uint8_t's
    EVENT_INDICATION_TLV_POWER_STATE_MODE_STATUS = 0x10,
    EVENT_INDICATION_TLV_POWER_STATE_MODE_CONFIG = 0x11,
    EVENT_INDICATION_TLV_IMS_CAPABILITY = 0x12,
};

/* External functions for the chat */
const char *dms_get_modem_revision();
const char *dms_get_modem_modem_serial_num();
const char *dms_get_modem_modem_hw_rev();
const char *dms_get_modem_modem_model();

void *dms_get_modem_info();
const char *get_dms_command(uint16_t msgid);

int handle_incoming_dms_message(uint8_t *buf, size_t buf_len);
#endif
