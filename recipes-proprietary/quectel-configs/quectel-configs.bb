inherit bin_package

DESCRIPTION = "Settings and calibration data for Quectel EC/EG2x modems"
LICENSE = "CLOSED"
PROVIDES = "quectel-configs"
MY_PN = "quectel-configs"
PR = "r1"
S = "${WORKDIR}"


INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SOLIBS = ".so"
FILES_SOLIBSDEV = ""
SRC_URI="file://etc/Headset_cal.acdb \
         file://etc/Hdmi_cal.acdb \
         file://etc/General_cal.acdb \
         file://etc/thermal-engine.conf \
         file://etc/izat.conf \
         file://etc/lowi.conf \
         file://etc/qmi_ip_cfg.xml \
         file://etc/Bluetooth_cal.acdb \
         file://etc/Speaker_cal.acdb \
         file://etc/Global_cal.acdb \
         file://etc/Handset_cal.acdb \
         file://etc/data/qmi_config.xml \
         file://etc/data/netmgr_config.xml \
         file://etc/data/dsi_config.xml \
         file://etc/gps.conf \
         file://data/mobileap_cfg.xml \
         file://data/qmi_ip_cfg.xml \
         file://data/dnsmasq.conf \
         file://etc/snd_soc_msm/snd_soc_msm \
         file://etc/snd_soc_msm/snd_soc_msm_2x \
         file://etc/snd_soc_msm/snd_soc_msm_2x_Fusion3 \
         file://etc/snd_soc_msm/snd_soc_msm_8x10_wcd \
         file://etc/snd_soc_msm/snd_soc_msm_8x10_wcd_skuaa \
         file://etc/snd_soc_msm/snd_soc_msm_8x10_wcd_skuab \
         file://etc/snd_soc_msm/snd_soc_msm_9x07_Tomtom_I2S \
         file://etc/snd_soc_msm/snd_soc_msm_9x40_Tomtom_I2S \
         file://etc/snd_soc_msm/snd_soc_msm_I2S \
         file://etc/snd_soc_msm/snd_soc_msm_Sitar \
         file://etc/snd_soc_msm/snd_soc_msm_Taiko_I2S \
         file://etc/snd_soc_msm/snd_soc_msm_Tapan \
         file://etc/snd_soc_msm/snd_soc_msm_TapanLite \
         file://etc/snd_soc_msm/snd_soc_msm_Tapan_SKUF \
         file://etc/snd_soc_msm/snd_soc_msm_Tasha_I2S"

do_install() {
      #make folders if they dont exist
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/etc/firmware
      install -d ${D}/etc/udhcpc.d
      install -d ${D}/etc/data
      install -d ${D}/etc/snd_soc_msm
      install -d ${D}/usr/persist
      # Calibration data
      cp ${S}/etc/Headset_cal.acdb ${D}/etc/
      cp ${S}/etc/Hdmi_cal.acdb ${D}/etc/
      cp ${S}/etc/General_cal.acdb ${D}/etc/
      cp ${S}/etc/Bluetooth_cal.acdb ${D}/etc/
      cp ${S}/etc/Speaker_cal.acdb ${D}/etc/
      cp ${S}/etc/Global_cal.acdb ${D}/etc/
      cp ${S}/etc/Handset_cal.acdb ${D}/etc/
      # Qualcomm IZat  & Location MQ settings
      cp ${S}/etc/izat.conf ${D}/etc/
      cp ${S}/etc/lowi.conf ${D}/etc/

      # ALSA config files
      cp ${S}/etc/snd_soc_msm/snd_soc_msm ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_2x ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3 ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_8x10_wcd ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_8x10_wcd_skuaa ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_8x10_wcd_skuab ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_9x07_Tomtom_I2S ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_9x40_Tomtom_I2S ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_I2S ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_Sitar ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_Taiko_I2S ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_Tapan ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_TapanLite ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_Tapan_SKUF ${D}/etc/snd_soc_msm/
      cp ${S}/etc/snd_soc_msm/snd_soc_msm_Tasha_I2S ${D}/etc/snd_soc_msm/

      cp ${S}/etc/qmi_ip_cfg.xml ${D}/etc/
      cp ${S}/etc/thermal-engine.conf ${D}/etc/

      cp ${S}/etc/data/qmi_config.xml ${D}/etc/data/
      cp ${S}/etc/data/netmgr_config.xml ${D}/etc/data/
      cp ${S}/etc/data/dsi_config.xml ${D}/etc/data/
      cp ${S}/etc/gps.conf ${D}/etc/data/

  
}

do_install_append() {
   
}
