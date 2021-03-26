#ifndef _DEVICES_H_
#define _DEVICES_H_

/* Devices */
/* Dynamic Port Mapper */
#define DPM_CTL "/dev/dpl_ctrl"
/* RMNET control device (what you see in the USB side of the modem) */
#define RMNET_CTL "/dev/rmnet_ctrl"
/* SMD Control device #8, where RMNET_CTRL is piped */
#define SMD_CNTL "/dev/smdcntl8"

/* Audio nodes */
#define SND_CTL "/dev/snd/controlC0"
#define PCM_DEV_VOCS                                                           \
  "/dev/snd/pcmC0D2" // Normal Voice calls use CS - Circuit Switch, device #2
#define PCM_DEV_VOLTE "/dev/snd/pcmC0D4" // VoLTE uses PCM device #4

#endif