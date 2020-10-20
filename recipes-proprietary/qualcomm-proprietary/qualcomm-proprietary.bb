inherit bin_package

DESCRIPTION = "Qualcomm Proprietary blobs and scripts"
LICENSE = "CLOSED"
PROVIDES = "qualcomm-proprietary"
RDEPENDS_${PN} = "glibc openssl"
MY_PN = "qualcomm-proprietary"
PR = "r1"
S = "${WORKDIR}/"


INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SOLIBS = ".so"
FILES_SOLIBSDEV = ""
SRC_URI="file://usr/bin/qmi_test_service_clnt_test_1000 \
         file://usr/bin/alsaucm_test \
         file://usr/bin/atfwd_daemon \
         file://usr/bin/diag_dci_sample \
         file://usr/bin/qti_ppp \
         file://usr/bin/DS_MUX \
         file://usr/bin/qmi_test_service_clnt_test_1001 \
         file://usr/bin/netmgrd \
         file://usr/bin/radish \
         file://usr/bin/time_daemon \
         file://usr/bin/psmd \
         file://usr/bin/qmi_test_service_clnt_test_0001 \
         file://usr/bin/eMBMs_TunnelingModule \
         file://usr/bin/qmi_test_service_clnt_test_0000 \
         file://usr/bin/thermal-engine \
         file://usr/bin/qmi_test_mt_client_init_instance \
         file://usr/bin/qmi_test_service_test \
         file://usr/bin/qmi_ip_multiclient \
         file://usr/bin/QCMAP_CLI \
         file://usr/bin/qmuxd \
         file://usr/bin/diag_callback_sample \
         file://usr/bin/diag_klog \
         file://usr/bin/qti \
         file://usr/bin/diag_uart_log \
         file://usr/bin/diag_mdlog \
         file://usr/bin/csd_server \
         file://usr/bin/qmi_shutdown_modem \
         file://usr/bin/port_bridge \
         file://usr/bin/test_diag \
         file://usr/bin/sendcal \
         file://usr/bin/diag_socket_log \
         file://usr/bin/qmi_simple_ril_test \
         file://usr/bin/mbimd \
         file://usr/bin/QCMAP_StaInterface \
         file://usr/bin/diagrebootapp \
         file://usr/bin/psm_test \
         file://usr/bin/QCMAP_ConnectionManager \
         file://usr/bin/qmi_test_service_clnt_test_2000 \
         file://usr/bin/PktRspTest \
         file://usr/bin/irsc_util \
         file://usr/bin/mbimd \
         file://usr/bin/location_hal_tests \
         file://usr/bin/lowi-test \
         file://usr/lib/libhardware.so.0.0.0  \
         file://usr/lib/libqcmap_client.so.1.0.0  \
         file://usr/lib/libdiag.so.1.0.0 \
         file://usr/lib/libadiertac.so.1.0.0 \
         file://usr/lib/libloc_mcm_test_shim.so.1.0.0 \
         file://usr/lib/libqdi.so.0.0.0 \
         file://usr/lib/libconfigdb.so.0.0.0 \
         file://usr/lib/libqcmap_cm.so.1.0.0 \
         file://usr/lib/libloc_net_iface.so.1.0.0 \
         file://usr/lib/libloc_pla.so.1.0.0 \
         file://usr/lib/libtime_genoff.so.1.0.0 \
         file://usr/lib/libpsm_client.so.0.0.0 \
         file://usr/lib/libloc_core.so.1.0.0 \
         file://usr/lib/libqdp.so.0.0.0 \
         file://usr/lib/libqmi_ip.so.1.0.0 \
         file://usr/lib/libacdbrtac.so.1.0.0 \
         file://usr/lib/libqmi_sap.so.1.0.0 \
         file://usr/lib/libaudcal.so.1.0.0 \
         file://usr/lib/libqmi_common_so.so.1.0.0 \
         file://usr/lib/libloc_stub.so.1.0.0 \
         file://usr/lib/libqmi_cci.so.1.0.0 \
         file://usr/lib/libloc_ds_api.so.1.0.0 \
         file://usr/lib/libgps_utils_so.so.1.0.0 \
         file://usr/lib/libqmi_encdec.so.1.0.0 \
         file://usr/lib/libqmi_csi.so.1.0.0 \
         file://usr/lib/libacdbmapper.so.1.0.0 \
         file://usr/lib/libacdbloader.so \
         file://usr/lib/libaudioalsa.so.1.0.0 \
         file://usr/lib/libloc_ext.so.1.0.0 \
         file://usr/lib/libqmi_client_helper.so.1.0.0 \
         file://usr/lib/libdsutils.so.1.0.0 \
         file://usr/lib/libloc_mcm_type_conv.so.1.0.0 \
         file://usr/lib/libqmiservices.so.1.0.0 \
         file://usr/lib/libqmi.so.1.0.0 \
         file://usr/lib/libqmiidl.so.1.0.0 \
         file://usr/lib/libdsi_netctrl.so.0.0.0 \
         file://usr/lib/libloc_eng_so.so.1.0.0 \
         file://usr/lib/libloc_hal_test_shim_extended.so.1.0.0 \
         file://usr/lib/libloc_mcm_qmi_test_shim.so.1.0.0 \
         file://usr/lib/libqmi_client_qmux.so.1.0.0 \
         file://usr/lib/libloc_mq_client.so.1.0.0 \
         file://usr/lib/libloc_hal_test_shim.so.1.0.0 \
         file://usr/lib/libnetmgr.so.0.0.0 \
         file://usr/lib/libxml.so.0.0.0 \
         file://usr/lib/libcutils.so.0.0.0 \
         file://usr/lib/libpsmutils.so.0.0.0 \
         file://usr/lib/liblog.so.0.0.0 \
         file://usr/lib/libpugixml.so.1.0.0 \
         file://usr/lib/libqcmaputils.so.1.0.0 \
         file://usr/lib/libizat_client_api.so.1.0.0 \
         file://usr/lib/liblocationservice.so.1.0.0 \
         file://usr/lib/liblocationservice_glue.so.1.0.0 \
         file://bin/dnsmasq_script.sh \
         file://etc/Headset_cal.acdb \
         file://etc/Hdmi_cal.acdb \
         file://etc/General_cal.acdb \
         file://etc/thermal-engine.conf \
         file://etc/izat.conf \
         file://etc/lowi.conf \
         file://etc/init.d \
         file://etc/init.d/data-init \
         file://etc/init.d/start_QCMAP_ConnectionManager_le \
         file://etc/init.d/start_atfwd_daemon \
         file://etc/init.d/csdserver \
         file://etc/init.d/netmgrd \
         file://etc/init.d/psmd \
         file://etc/init.d/thermal-engine \
         file://etc/init.d/start_stop_qti_ppp_le \
         file://etc/init.d/start_at_cmux_le \
         file://etc/init.d/start_stop_qmi_ip_multiclient \
         file://etc/init.d/qmuxd \
         file://etc/init.d/port_bridge \
         file://etc/init.d/time_serviced \
         file://etc/init.d/mssboot \
         file://etc/init.d/start_qti_le \
         file://etc/init.d/start_eMBMs_TunnelingModule_le \
         file://etc/init.d/diagrebootapp \
         file://etc/init.d/init_irsc_util \
         file://etc/udhcpc.d \
         file://etc/udhcpc.d/udhcpc.script \
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
      install -d ${D}/bin
      install -d ${D}/lib
      install -d ${D}/usr/bin
      install -d ${D}/usr/lib
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/etc/firmware
      install -d ${D}/etc/udhcpc.d
      install -d ${D}/etc/data
      install -d ${D}/etc/snd_soc_msm

      install -d ${D}/usr/persist

      # binaries
      install -m 0755 ${S}/usr/bin/alsaucm_test ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/atfwd_daemon ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_1000 ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_dci_sample ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qti_ppp ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/DS_MUX ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_1001 ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/netmgrd ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/radish ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/time_daemon ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/psmd ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/mbimd ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_0001 ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/eMBMs_TunnelingModule ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_0000 ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/thermal-engine ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_mt_client_init_instance ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_test ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_ip_multiclient ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/QCMAP_CLI ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmuxd ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_callback_sample ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_klog ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qti ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_uart_log ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_mdlog ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_shutdown_modem ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/port_bridge ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/test_diag ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/sendcal ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diag_socket_log ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_simple_ril_test ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/mbimd ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/QCMAP_StaInterface ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/diagrebootapp ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/psm_test ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/QCMAP_ConnectionManager ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_2000 ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/PktRspTest ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/irsc_util ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/csd_server ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/location_hal_tests ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/lowi-test ${D}/usr/bin

      # Libraries
      cp ${S}/usr/lib/liblog.so.0.0.0   ${D}/usr/lib/
      cp ${S}/usr/lib/libpugixml.so.1.0.0   ${D}/usr/lib/
      cp ${S}/usr/lib/libqcmaputils.so.1.0.0   ${D}/usr/lib/
      cp ${S}/usr/lib/libqcmap_client.so.1.0.0   ${D}/usr/lib/
      cp ${S}/usr/lib/libdiag.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libadiertac.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_mcm_test_shim.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqdi.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libconfigdb.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqcmap_cm.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_net_iface.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_pla.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libtime_genoff.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libpsm_client.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libhardware.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_core.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqdp.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_ip.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libacdbrtac.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_sap.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libaudcal.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_common_so.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_stub.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_cci.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_ds_api.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libgps_utils_so.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_encdec.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_csi.so.1.0.0  ${D}/usr/lib/
    #  cp ${S}/usr/lib/libloc_net_iface.la  ${D}/usr/lib/
      cp ${S}/usr/lib/libacdbmapper.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libacdbloader.so  ${D}/usr/lib/
      cp ${S}/usr/lib/libaudioalsa.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_ext.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_client_helper.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libdsutils.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_mcm_type_conv.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmiservices.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmiidl.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libdsi_netctrl.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_eng_so.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_hal_test_shim_extended.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_mcm_qmi_test_shim.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_client_qmux.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_mq_client.so.1.0.0  ${D}/usr/lib/
   #   cp ${S}/usr/lib/libloc_eng_so.la  ${D}/usr/lib/
      cp ${S}/usr/lib/libloc_hal_test_shim.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libnetmgr.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libxml.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libpsmutils.so.0.0.0 ${D}/usr/lib/
      cp ${S}/usr/lib/libcutils.so.0.0.0 ${D}/usr/lib/
      cp ${S}/usr/lib/libizat_client_api.so.1.0.0 ${D}/usr/lib/
      cp ${S}/usr/lib/liblocationservice.so.1.0.0 ${D}/usr/lib/
      cp ${S}/usr/lib/liblocationservice_glue.so.1.0.0 ${D}/usr/lib/

      # scripts and init
      install -m 0755 ${S}/bin/dnsmasq_script.sh ${D}/bin/

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

      # Daemon settings
      cp ${S}/etc/udhcpc.d/udhcpc.script ${D}/etc/udhcpc.d/
     # cp ${S}/etc/udhcpc.d/50default ${D}/etc/udhcpc.d/
      cp ${S}/etc/qmi_ip_cfg.xml ${D}/etc/
      cp ${S}/etc/thermal-engine.conf ${D}/etc/

      cp ${S}/etc/data/qmi_config.xml ${D}/etc/data/
      cp ${S}/etc/data/netmgr_config.xml ${D}/etc/data/
      cp ${S}/etc/data/dsi_config.xml ${D}/etc/data/
      cp ${S}/etc/gps.conf ${D}/etc/data/

      # services
      install -m 0755 ${S}/etc/init.d/data-init ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/netmgrd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/psmd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/thermal-engine ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_at_cmux_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_stop_qmi_ip_multiclient ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/qmuxd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/port_bridge ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/time_serviced ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_qti_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_eMBMs_TunnelingModule_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/diagrebootapp ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/init_irsc_util ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_atfwd_daemon ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/csdserver ${D}/etc/init.d/

      # link services on boot
      ln -sf -r ${D}/etc/init.d/data-init ${D}/etc/rcS.d/S97data-init
      ln -sf -r ${D}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/rcS.d/S39start_QCMAP_ConnectionManager_le
      ln -sf -r ${D}/etc/init.d/netmgrd ${D}/etc/rcS.d/S45netmgrd
      ln -sf -r ${D}/etc/init.d/psmd ${D}/etc/rcS.d/S36psmd
      ln -sf -r ${D}/etc/init.d/thermal-engine ${D}/etc/rcS.d/S40thermal-engine
      # ln -sf -r ${D}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/rcS.d/S
      ln -sf -r ${D}/etc/init.d/start_at_cmux_le ${D}/etc/rcS.d/S43start_at_cmux_le
      # ln -sf -r ${D}/etc/init.d/start_stop_qmi_ip_multiclient ${D}/etc/rcS.d/Sx
      ln -sf -r ${D}/etc/init.d/qmuxd ${D}/etc/rcS.d/S40qmuxd
      ln -sf -r ${D}/etc/init.d/port_bridge ${D}/etc/rcS.d/S38port_bridge
      ln -sf -r ${D}/etc/init.d/time_serviced ${D}/etc/rcS.d/S29time_serviced
      ln -sf -r ${D}/etc/init.d/mssboot ${D}/etc/rcS.d/S30mssboot
      ln -sf -r ${D}/etc/init.d/start_qti_le ${D}/etc/rcS.d/S40start_qti_le
      ln -sf -r ${D}/etc/init.d/start_eMBMs_TunnelingModule_le ${D}/etc/rcS.d/S70start_eMBMs_TunnelingModule_le
      # ln -sf -r ${D}/etc/init.d/diagrebootapp ${D}/etc/rcS.d/S
      ln -sf -r ${D}/etc/init.d/init_irsc_util ${D}/etc/rcS.d/S29init_irsc_util
      # If you need the console in the serial port disable this:
      ln -sf -r ${D}/etc/init.d/start_atfwd_daemon ${D}/etc/rcS.d/S80start_atfwd_daemon
      ln -sf -r ${D}/etc/init.d/csdserver ${D}/etc/rcS.d/S45csdserver
}

do_install_append() {
      ln -sf -r  ${D}/usr/lib/liblog.so.0.0.0 ${D}/usr/lib/liblog.so.0
      ln -sf -r  ${D}/usr/lib/libpugixml.so.1.0.0 ${D}/usr/lib/libpugixml.so.1
      ln -sf -r  ${D}/usr/lib/libqcmaputils.so.1.0.0 ${D}/usr/lib/libqcmaputils.so.1
      ln -sf -r  ${D}/usr/lib/libqcmap_client.so.1.0.0 ${D}/usr/lib/libqcmap_client.so.1
      ln -sf -r  ${D}/usr/lib/libdiag.so.1.0.0 ${D}/usr/lib/libdiag.so.1
      ln -sf -r  ${D}/usr/lib/libadiertac.so.1.0.0 ${D}/usr/lib/libadiertac.so.1
      ln -sf -r  ${D}/usr/lib/libloc_mcm_test_shim.so.1.0.0 ${D}/usr/lib/libloc_mcm_test_shim.so.1
      ln -sf -r  ${D}/usr/lib/libqdi.so.0.0.0 ${D}/usr/lib/libqdi.so.0
      ln -sf -r  ${D}/usr/lib/libconfigdb.so.0.0.0 ${D}/usr/lib/libconfigdb.so.0
      ln -sf -r  ${D}/usr/lib/libqcmap_cm.so.1.0.0 ${D}/usr/lib/libqcmap_cm.so.1
      ln -sf -r  ${D}/usr/lib/libloc_net_iface.so.1.0.0 ${D}/usr/lib/libloc_net_iface.so.1
      ln -sf -r  ${D}/usr/lib/libloc_pla.so.1.0.0 ${D}/usr/lib/libloc_pla.so.1
      ln -sf -r  ${D}/usr/lib/libtime_genoff.so.1.0.0 ${D}/usr/lib/libtime_genoff.so.1
      ln -sf -r  ${D}/usr/lib/libpsm_client.so.0.0.0 ${D}/usr/lib/libpsm_client.so.0
      ln -sf -r  ${D}/usr/lib/libloc_core.so.1.0.0 ${D}/usr/lib/libloc_core.so.1
      ln -sf -r  ${D}/usr/lib/libqdp.so.0.0.0 ${D}/usr/lib/libqdp.so.0
      ln -sf -r  ${D}/usr/lib/libqmi_ip.so.1.0.0 ${D}/usr/lib/libqmi_ip.so.1
      ln -sf -r  ${D}/usr/lib/libacdbrtac.so.1.0.0 ${D}/usr/lib/libacdbrtac.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_sap.so.1.0.0 ${D}/usr/lib/libqmi_sap.so.1
      ln -sf -r  ${D}/usr/lib/libaudcal.so.1.0.0 ${D}/usr/lib/libaudcal.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_common_so.so.1.0.0 ${D}/usr/lib/libqmi_common_so.so.1
      ln -sf -r  ${D}/usr/lib/libloc_stub.so.1.0.0 ${D}/usr/lib/libloc_stub.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_cci.so.1.0.0 ${D}/usr/lib/libqmi_cci.so.1
      ln -sf -r  ${D}/usr/lib/libloc_ds_api.so.1.0.0 ${D}/usr/lib/libloc_ds_api.so.1
      ln -sf -r  ${D}/usr/lib/libgps_utils_so.so.1.0.0 ${D}/usr/lib/libgps_utils_so.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_encdec.so.1.0.0 ${D}/usr/lib/libqmi_encdec.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_csi.so.1.0.0 ${D}/usr/lib/libqmi_csi.so.1
      ln -sf -r  ${D}/usr/lib/libacdbmapper.so.1.0.0 ${D}/usr/lib/libacdbmapper.so.1
      ln -sf -r  ${D}/usr/lib/libaudioalsa.so.1.0.0 ${D}/usr/lib/libaudioalsa.so.1
      ln -sf -r  ${D}/usr/lib/libloc_ext.so.1.0.0 ${D}/usr/lib/libloc_ext.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_client_helper.so.1.0.0 ${D}/usr/lib/libqmi_client_helper.so.1
      #ln -sf -r  ${D}/usr/lib/llibdsutils.so.1.0.0 ${D}/usr/lib/libdsutils.so.1
      ln -sf -r  ${D}/usr/lib/libloc_mcm_type_conv.so.1.0.0 ${D}/usr/lib/libloc_mcm_type_conv.so.1
      ln -sf -r  ${D}/usr/lib/libqmiservices.so.1.0.0 ${D}/usr/lib/libqmiservices.so.1
      ln -sf -r  ${D}/usr/lib/libqmi.so.1.0.0 ${D}/usr/lib/libqmi.so.1
      ln -sf -r  ${D}/usr/lib/libqmiidl.so.1.0.0 ${D}/usr/lib/libqmiidl.so.1
      ln -sf -r  ${D}/usr/lib/libdsi_netctrl.so.0.0.0 ${D}/usr/lib/libdsi_netctrl.so.0
      ln -sf -r  ${D}/usr/lib/libloc_eng_so.so.1.0.0 ${D}/usr/lib/libloc_eng_so.so.1
      ln -sf -r  ${D}/usr/lib/libloc_hal_test_shim_extended.so.1.0.0 ${D}/usr/lib/libloc_hal_test_shim_extended.so.1
      ln -sf -r  ${D}/usr/lib/libloc_mcm_qmi_test_shim.so.1.0.0 ${D}/usr/lib/libloc_mcm_qmi_test_shim.so.1
      ln -sf -r  ${D}/usr/lib/libqmi_client_qmux.so.1.0.0 ${D}/usr/lib/libqmi_client_qmux.so.1
      ln -sf -r  ${D}/usr/lib/libloc_mq_client.so.1.0.0 ${D}/usr/lib/libloc_mq_client.so.1
      ln -sf -r  ${D}/usr/lib/libloc_hal_test_shim.so.1.0.0 ${D}/usr/lib/libloc_hal_test_shim.so.1
      ln -sf -r  ${D}/usr/lib/libnetmgr.so.0.0.0 ${D}/usr/lib/libnetmgr.so.0
      ln -sf -r  ${D}/usr/lib/libxml.so.0.0.0 ${D}/usr/lib/libxml.so.0
      ln -sf -r  ${D}/usr/lib/libpsmutils.so.0.0.0 ${D}/usr/lib/libpsmutils.so.0
      ln -sf -r  ${D}/usr/lib/libcutils.so.0.0.0 ${D}/usr/lib/libcutils.so.0
     ln -sf -r  ${D}/usr/lib/libdsutils.so.1.0.0 ${D}/usr/lib/libdsutils.so.1
     ln -sf -r  ${D}/usr/lib/libhardware.so.0.0.0 ${D}/usr/lib/libhardware.so.0
     ln -sf -r  ${D}/usr/lib/libizat_client_api.so.1.0.0 ${D}/usr/lib/libizat_client_api.so.1
     ln -sf -r  ${D}/usr/lib/liblocationservice.so.1.0.0 ${D}/usr/lib/liblocationservice.so.1
     ln -sf -r  ${D}/usr/lib/liblocationservice_glue.so.1.0.0 ${D}/usr/lib/liblocationservice_glue.so.1
}
