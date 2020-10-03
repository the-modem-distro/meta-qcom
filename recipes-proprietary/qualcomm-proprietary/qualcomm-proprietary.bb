inherit bin_package

DESCRIPTION = "Qualcomm Proprietary blobs and scripts"
LICENSE = "CLOSED"
PROVIDES = "qualcomm-proprietary"
RDEPENDS_${PN} = "glibc openssl"
MY_PN = "qualcomm-proprietary"
PR = "r1"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://usr/bin/qmi_test_service_clnt_test_1000 \
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
         file://usr/lib/libacdbloader.so.1.0.0 \
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
         file://lib/libcrypt.so.1 \
         file://bin/dnsmasq_script.sh \
         file://etc/Headset_cal.acdb \
         file://etc/Hdmi_cal.acdb \
         file://etc/General_cal.acdb \
         file://etc/thermal-engine.conf \
         file://etc/init.d \
         file://etc/init.d/data-init \
         file://etc/init.d/start_QCMAP_ConnectionManager_le \
         file://etc/init.d/start_atfwd_daemon \
         file://etc/init.d/netmgrd \
         file://etc/init.d/psmd \
         file://etc/init.d/thermal-engine \
         file://etc/init.d/start_stop_qti_ppp_le \
         file://etc/init.d/start_at_cmux_le \
         file://etc/init.d/start_stop_qmi_ip_multiclient \
         file://etc/init.d/qmuxd \
         file://etc/init.d/port_bridge \
         file://etc/init.d/time_serviced \
         file://etc/init.d/start_qti_le \
         file://etc/init.d/start_eMBMs_TunnelingModule_le \
         file://etc/init.d/diagrebootapp \
         file://etc/init.d/irsc_util \
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
         file://data/dnsmasq.conf"

# file://etc/firmware/wcd9310/wcd9310_mbhc.bin 
# file://etc/firmware/wcd9310/wcd9310_anc.bin 

S = "${WORKDIR}/"

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

      install -d ${D}/usr/persist
 
      # binaries
      cp ${S}/usr/bin/atfwd_daemon ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_clnt_test_1000 ${D}/usr/bin
      cp ${S}/usr/bin/diag_dci_sample ${D}/usr/bin
      cp ${S}/usr/bin/qti_ppp ${D}/usr/bin
      cp ${S}/usr/bin/DS_MUX ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_clnt_test_1001 ${D}/usr/bin
      cp ${S}/usr/bin/netmgrd ${D}/usr/bin
      cp ${S}/usr/bin/radish ${D}/usr/bin
      cp ${S}/usr/bin/time_daemon ${D}/usr/bin
      cp ${S}/usr/bin/psmd ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_clnt_test_0001 ${D}/usr/bin
      cp ${S}/usr/bin/eMBMs_TunnelingModule ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_clnt_test_0000 ${D}/usr/bin
      cp ${S}/usr/bin/thermal-engine ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_mt_client_init_instance ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_test ${D}/usr/bin
      cp ${S}/usr/bin/qmi_ip_multiclient ${D}/usr/bin
      cp ${S}/usr/bin/QCMAP_CLI ${D}/usr/bin
      cp ${S}/usr/bin/qmuxd ${D}/usr/bin
      cp ${S}/usr/bin/diag_callback_sample ${D}/usr/bin
      cp ${S}/usr/bin/diag_klog ${D}/usr/bin
      cp ${S}/usr/bin/qti ${D}/usr/bin
      cp ${S}/usr/bin/diag_uart_log ${D}/usr/bin
      cp ${S}/usr/bin/diag_mdlog ${D}/usr/bin
      cp ${S}/usr/bin/qmi_shutdown_modem ${D}/usr/bin
      cp ${S}/usr/bin/port_bridge ${D}/usr/bin
      cp ${S}/usr/bin/test_diag ${D}/usr/bin
      cp ${S}/usr/bin/sendcal ${D}/usr/bin
      cp ${S}/usr/bin/diag_socket_log ${D}/usr/bin
      cp ${S}/usr/bin/qmi_simple_ril_test ${D}/usr/bin
      cp ${S}/usr/bin/mbimd ${D}/usr/bin
      cp ${S}/usr/bin/QCMAP_StaInterface ${D}/usr/bin
      cp ${S}/usr/bin/diagrebootapp ${D}/usr/bin
      cp ${S}/usr/bin/psm_test ${D}/usr/bin
      cp ${S}/usr/bin/QCMAP_ConnectionManager ${D}/usr/bin
      cp ${S}/usr/bin/qmi_test_service_clnt_test_2000 ${D}/usr/bin
      cp ${S}/usr/bin/PktRspTest ${D}/usr/bin
      cp ${S}/usr/bin/irsc_util ${D}/usr/bin
      
      # Set executable flags
      chmod +x ${D}/usr/bin/qmi_test_service_clnt_test_1000
      chmod +x ${D}/usr/bin/diag_dci_sample
      chmod +x ${D}/usr/bin/qti_ppp
      chmod +x ${D}/usr/bin/DS_MUX
      chmod +x ${D}/usr/bin/qmi_test_service_clnt_test_1001
      chmod +x ${D}/usr/bin/netmgrd
      chmod +x ${D}/usr/bin/radish
      chmod +x ${D}/usr/bin/time_daemon
      chmod +x ${D}/usr/bin/psmd
      chmod +x ${D}/usr/bin/qmi_test_service_clnt_test_0001
      chmod +x ${D}/usr/bin/eMBMs_TunnelingModule
      chmod +x ${D}/usr/bin/qmi_test_service_clnt_test_0000
      chmod +x ${D}/usr/bin/thermal-engine
      chmod +x ${D}/usr/bin/qmi_test_mt_client_init_instance
      chmod +x ${D}/usr/bin/qmi_test_service_test
      chmod +x ${D}/usr/bin/qmi_ip_multiclient
      chmod +x ${D}/usr/bin/QCMAP_CLI
      chmod +x ${D}/usr/bin/qmuxd
      chmod +x ${D}/usr/bin/diag_callback_sample
      chmod +x ${D}/usr/bin/diag_klog
      chmod +x ${D}/usr/bin/qti
      chmod +x ${D}/usr/bin/diag_uart_log
      chmod +x ${D}/usr/bin/diag_mdlog
      chmod +x ${D}/usr/bin/qmi_shutdown_modem
      chmod +x ${D}/usr/bin/port_bridge
      chmod +x ${D}/usr/bin/test_diag
      chmod +x ${D}/usr/bin/sendcal
      chmod +x ${D}/usr/bin/diag_socket_log
      chmod +x ${D}/usr/bin/qmi_simple_ril_test
      chmod +x ${D}/usr/bin/mbimd
      chmod +x ${D}/usr/bin/QCMAP_StaInterface
      chmod +x ${D}/usr/bin/diagrebootapp
      chmod +x ${D}/usr/bin/psm_test
      chmod +x ${D}/usr/bin/QCMAP_ConnectionManager
      chmod +x ${D}/usr/bin/qmi_test_service_clnt_test_2000
      chmod +x ${D}/usr/bin/PktRspTest
      chmod +x ${D}/usr/bin/irsc_util
      chmod +x ${D}/usr/bin/atfwd_daemon

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
    #  cp ${S}/usr/lib/libloc_ds_api.la  ${D}/usr/lib/
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
      cp ${S}/usr/lib/libacdbloader.so.1.0.0  ${D}/usr/lib/
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
      cp ${S}/lib/libcrypt.so.1 ${D}/lib/

      # Library simlinks... tbd
      ln -sf -r  ${D}/usr/lib/liblog.so.0.0.0 ${D}/usr/lib/liblog.so.0

      # scripts and init
      cp ${S}/bin/dnsmasq_script.sh ${D}/bin/

      # Calibration data
      cp ${S}/etc/Headset_cal.acdb ${D}/etc/
      cp ${S}/etc/Hdmi_cal.acdb ${D}/etc/
      cp ${S}/etc/General_cal.acdb ${D}/etc/
      cp ${S}/etc/Bluetooth_cal.acdb ${D}/etc/
      cp ${S}/etc/Speaker_cal.acdb ${D}/etc/
      cp ${S}/etc/Global_cal.acdb ${D}/etc/
      cp ${S}/etc/Handset_cal.acdb ${D}/etc/

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
      cp ${S}/etc/init.d/data-init ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/init.d/
      cp ${S}/etc/init.d/netmgrd ${D}/etc/init.d/
      cp ${S}/etc/init.d/psmd ${D}/etc/init.d/
      cp ${S}/etc/init.d/thermal-engine ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_at_cmux_le ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_stop_qmi_ip_multiclient ${D}/etc/init.d/
      cp ${S}/etc/init.d/qmuxd ${D}/etc/init.d/
      cp ${S}/etc/init.d/port_bridge ${D}/etc/init.d/
      cp ${S}/etc/init.d/time_serviced ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_qti_le ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_eMBMs_TunnelingModule_le ${D}/etc/init.d/
      cp ${S}/etc/init.d/diagrebootapp ${D}/etc/init.d/
      cp ${S}/etc/init.d/irsc_util ${D}/etc/init.d/
      cp ${S}/etc/init.d/start_atfwd_daemon ${D}/etc/init.d/

      # link services on boot
      ln -sf -r ${D}/etc/init.d/data-init ${D}/etc/rcS.d/S97data-init
      ln -sf -r ${D}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/rcS.d/S39start_QCMAP_ConnectionManager_le
      ln -sf -r ${D}/etc/init.d/netmgrd ${D}/etc/rcS.d/S45netmgrd
      ln -sf -r ${D}/etc/init.d/psmd ${D}/etc/rcS.d/S36psmd
      ln -sf -r ${D}/etc/init.d/thermal-engine ${D}/etc/rcS.d/S40thermal-engine
      # ln -sf -r ${D}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/rcS.d/S
      ln -sf -r ${D}/etc/init.d/start_at_cmux_le ${D}/etc/rcS.d/S43start_at_cmux_le
      # ln -sf -r ${D}/etc/init.d/start_stop_qmi_ip_multiclient ${D}/etc/rcS.d/S
      ln -sf -r ${D}/etc/init.d/qmuxd ${D}/etc/rcS.d/S40qmuxd
      ln -sf -r ${D}/etc/init.d/port_bridge ${D}/etc/rcS.d/S38port_bridge
      ln -sf -r ${D}/etc/init.d/time_serviced ${D}/etc/rcS.d/S29time_serviced
      ln -sf -r ${D}/etc/init.d/start_qti_le ${D}/etc/rcS.d/S40start_qti_le
      ln -sf -r ${D}/etc/init.d/start_eMBMs_TunnelingModule_le ${D}/etc/rcS.d/S70start_eMBMs_TunnelingModule_le
      # ln -sf -r ${D}/etc/init.d/diagrebootapp ${D}/etc/rcS.d/S
      ln -sf -r ${D}/etc/init.d/irsc_util ${D}/etc/rcS.d/S29init_irsc_util
      ln -sf -r ${D}/etc/init.d/start_atfwd_daemon ${D}/etc/rcS.d/S80start_atfwd_daemon

      # Mark init script as executable
      chmod +x ${D}/etc/init.d/data-init 
      chmod +x ${D}/etc/init.d/start_QCMAP_ConnectionManager_le 
      chmod +x ${D}/etc/init.d/netmgrd 
      chmod +x ${D}/etc/init.d/psmd 
      chmod +x ${D}/etc/init.d/thermal-engine 
      chmod +x ${D}/etc/init.d/start_stop_qti_ppp_le 
      chmod +x ${D}/etc/init.d/start_at_cmux_le 
      chmod +x ${D}/etc/init.d/start_stop_qmi_ip_multiclient 
      chmod +x ${D}/etc/init.d/qmuxd 
      chmod +x ${D}/etc/init.d/port_bridge 
      chmod +x ${D}/etc/init.d/time_serviced 
      chmod +x ${D}/etc/init.d/start_qti_le 
      chmod +x ${D}/etc/init.d/start_eMBMs_TunnelingModule_le 
      chmod +x ${D}/etc/init.d/diagrebootapp 
      chmod +x ${D}/etc/init.d/irsc_util 


}
