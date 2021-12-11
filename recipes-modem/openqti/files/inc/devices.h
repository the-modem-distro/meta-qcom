/* SPDX-License-Identifier: MIT */

#ifndef _DEVICES_H_
#define _DEVICES_H_

/* Devices */
/* Dynamic Port Mapper */
#define DPM_CTL "/dev/dpl_ctrl"
/* RMNET control device (what you see in the USB side of the modem) */
#define RMNET_CTL "/dev/rmnet_ctrl"
/* SMD Control device #8, where RMNET_CTRL is piped */
#define SMD_CNTL "/dev/smdcntl8"
/* SMD Device 7: ADSP side GPS port */
#define SMD_GPS "/dev/smd7"
/* SMD Data 3 is used by Quectel's LPM kernel driver */
#define SMD_DATA3 "/dev/smd8"

/* ttyGS0: USB side GPS port */
#define USB_GPS "/dev/ttyGS0"

/* Audio nodes */
#define SND_CTL "/dev/snd/controlC0"
#define PCM_DEV_VOCS                                                           \
  "/dev/snd/pcmC0D2" // Normal Voice calls use CS - Circuit Switch, device #2
#define PCM_DEV_VOLTE "/dev/snd/pcmC0D4" // VoLTE uses PCM device #4

#define EXTERNAL_CODEC_DETECT_PATH "/sys/devices/78b6000.i2c/i2c-2/2-001b/rt5616_detected_state"
#define USB_SERIAL_TRANSPORTS_PATH                                             \
  "/sys/class/android_usb/android0/f_serial/transports"
#define USB_FUNC_PATH "/sys/class/android_usb/android0/functions"
#define USB_EN_PATH "/sys/class/android_usb/android0/enable"
#define USB_EN_PATH "/sys/class/android_usb/android0/enable"
#endif