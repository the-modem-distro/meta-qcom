inherit bin_package

DESCRIPTION = "Quectel Proprietary blobs and scripts"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "quectel-proprietary"
MY_PN = "quectel-proprietary"
PR = "r1"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

SRC_URI="file://etc/init.d/start_ipacm_perf_le \
        file://etc/init.d/start_shortcut_fe_le \
        file://etc/init.d/start_pdcd \
        file://etc/init.d/start_sdcard \
        file://etc/init.d/start_atfwd_daemon \
        file://etc/init.d/start_QCMAP_ConnectionManager_le \
        file://etc/init.d/start_ipacmdiag_le \
        file://etc/init.d/init_irsc_util \
        file://etc/init.d/start_embms_le \
        file://etc/init.d/start_stop_qti_ppp_le \
        file://etc/init.d/qmuxd \
        file://etc/init.d/netmgrd \
        file://etc/init.d/start_qti_le \
        file://etc/init.d/qmi_shutdown_modemd \
        file://etc/init.d/start_alsa_daemon \
        file://etc/init.d/psmd \
        file://usr/bin/irsc_util \
        file://usr/bin/qmi_ping_clnt_test_2000 \
        file://usr/bin/qmi_test_service_clnt_test_1001 \
        file://usr/bin/qmi_test_service_test \
        file://usr/bin/qmi_test_service_clnt_test_1000 \
        file://usr/bin/alsa_daemon \
        file://usr/bin/rmnetcli \
        file://usr/bin/qti \
        file://usr/bin/qmi_test_mt_client_init_instance \
        file://usr/bin/diag_socket_log \
        file://usr/bin/alsaucm_test \
        file://usr/bin/QCMAP_StaInterface \
        file://usr/bin/qmi_test_service_clnt_test_0001 \
        file://usr/bin/qmi_shutdown_modem \
        file://usr/bin/audio_ftm \
        file://usr/bin/netmgrd \
        file://usr/bin/qmi_test_service_clnt_test_0000 \
        file://usr/bin/qti_ppp \
        file://usr/bin/qmi_ping_clnt_test_0000 \
        file://usr/bin/diag_uart_log \
        file://usr/bin/qmi_simple_ril_test \
        file://usr/bin/psm_test \
        file://usr/bin/location_hal_test \
        file://usr/bin/radish \
        file://usr/bin/qmi_ping_test \
        file://usr/bin/diag_mdlog \
        file://usr/bin/qmi_ping_clnt_test_0001 \
        file://usr/bin/qmuxd \
        file://usr/bin/psmd \
        file://usr/bin/diagrebootapp \
        file://usr/bin/diag_klog \
        file://usr/bin/qmi_test_service_clnt_test_2000 \
        file://usr/bin/diag_dci_sample \
        file://usr/bin/qmi_ping_clnt_test_1000 \
        file://usr/bin/QCMAP_ConnectionManager \
        file://usr/bin/qmi_ping_clnt_test_1001 \
        file://usr/bin/amix \
        file://usr/bin/atfwd_daemon \
        file://usr/bin/qmi_ping_svc \
        file://usr/bin/diag_callback_sample \
        file://usr/lib/libloc_stub.so.1.0.0 \
        file://usr/lib/libdiag.so.1.0.0 \
        file://usr/lib/libqcmap_cm.so.1.0.0 \
        file://usr/lib/libqcmaputils.so.1.0.0 \
        file://usr/lib/libfaad2.so.1.0.0 \
        file://usr/lib/libqcmapipc.so.1.0.0 \
        file://usr/lib/libdsutils.so.1.0.0 \
        file://usr/lib/libamrnb.so.1.0.0 \
        file://usr/lib/libpsm_client.so.0.0.0 \
        file://usr/lib/libconfigdb.so.0.0.0 \
        file://usr/lib/libmp4ff.so.1.0.0 \
        file://usr/lib/libgps_default_so.so.1.0.0 \
        file://usr/lib/libpugixml.so.1.0.0 \
        file://usr/lib/libmad.so.1.0.0 \
        file://usr/lib/libqmiservices.so.1.0.0 \
        file://usr/lib/libqmi.so.1.0.0 \
        file://usr/lib/libpsmutils.so.0.0.0 \
        file://usr/lib/libalsa_intf.so.1.0.0 \
        file://usr/lib/libqmi_csi.so.1.0.0 \
        file://usr/lib/libtts_data.so.1.0.0 \
        file://usr/lib/libnetmgr.so.0.0.0 \
        file://usr/lib/libqmi_client_helper.so.1.0.0 \
        file://usr/lib/libacdbloader.so.1.0.0 \
        file://usr/lib/libqmi_encdec.so.1.0.0 \
        file://usr/lib/libqmi_client_qmux.so.1.0.0 \
        file://usr/lib/libflp_default.so.1.0.0 \
        file://usr/lib/libfdk-aac.so.1.0.0 \
        file://usr/lib/libdsi_netctrl.so.0.0.0 \
        file://usr/lib/librmnetctl.so.0.0.0 \
        file://usr/lib/libqdi.so.0.0.0 \
        file://usr/lib/libqmi_common_so.so.1.0.0 \
        file://usr/lib/libqmi_sap.so.1.0.0 \
        file://usr/lib/libqmiidl.so.1.0.0 \
        file://usr/lib/libtime_genoff.so.1.0.0 \
        file://usr/lib/libxml.so.0.0.0 \
        file://usr/lib/liblog.so.0.0.0 \
        file://usr/lib/libacdbrtac.so.1.0.0 \
        file://usr/lib/libadiertac.so.1.0.0 \
        file://usr/lib/libaudcal.so.1.0.0 \
        file://usr/lib/libaudioalsa.so.1.0.0 \
        file://usr/lib/libacdbmapper.so.1.0.0 \
        file://usr/lib/libqmi_sap.so.1.0.0 \
        file://usr/lib/libqmi_cci.so.1.0.0"

S = "${WORKDIR}"

do_install() {
        #make folders if they dont exist
        install -d ${D}/usr/bin
        install -d ${D}/usr/lib
        install -d ${D}/etc/init.d
        install -d ${D}/etc/rcS.d
        # binaries
        install -m 0755 ${S}/usr/bin/irsc_util ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_clnt_test_2000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_1001 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_test ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_1000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/alsa_daemon ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/rmnetcli ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qti ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_mt_client_init_instance ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_socket_log ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/alsaucm_test ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/QCMAP_StaInterface ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_0001 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_shutdown_modem ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/audio_ftm ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/netmgrd ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_0000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qti_ppp ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_clnt_test_0000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_uart_log ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_simple_ril_test ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/psm_test ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/location_hal_test  ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/radish ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_test ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_mdlog ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_clnt_test_0001 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmuxd ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/psmd ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diagrebootapp ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_klog ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_test_service_clnt_test_2000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_dci_sample ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_clnt_test_1000 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/QCMAP_ConnectionManager ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_clnt_test_1001 ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/amix ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/atfwd_daemon ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/qmi_ping_svc ${D}/usr/bin/
        install -m 0755 ${S}/usr/bin/diag_callback_sample ${D}/usr/bin/

      # Init scripts
      install -m 0755 ${S}/etc/init.d/start_ipacm_perf_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_shortcut_fe_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_pdcd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_sdcard ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_atfwd_daemon ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_ipacmdiag_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/init_irsc_util ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_embms_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/qmuxd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_qti_le ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/qmi_shutdown_modemd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/start_alsa_daemon ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/netmgrd ${D}/etc/init.d/
      install -m 0755 ${S}/etc/init.d/psmd ${D}/etc/init.d/

      # Shared libraries
      install -m 0644 ${S}/usr/lib/libqcmaputils.so.1.0.0 ${D}/usr/lib/
      #install -m 0644 ${S}/usr/lib/libpthread.so.0.0.0 ${D}/usr/lib/
      #install -m 0644 ${S}/usr/lib/libc.so.6.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libdiag.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libfaad2.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmiservices.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libtts_data.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libpugixml.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libamrnb.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_csi.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqcmapipc.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libdsi_netctrl.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_client_helper.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libloc_stub.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/librmnetctl.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_encdec.so.1.0.0 ${D}/usr/lib/
     # install -m 0644 ${S}/usr/lib/librt.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libmad.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libalsa_intf.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqcmap_cm.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_cci.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_client_qmux.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libmp4ff.so.1.0.0 ${D}/usr/lib/
      #install -m 0644 ${S}/usr/lib/libgcc_s.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libflp_default.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libconfigdb.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libdsutils.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libfdk-aac.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libacdbloader.so.1.0.0 ${D}/usr/lib/
      #install -m 0644 ${S}/usr/lib/libglib-2.0.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libpsmutils.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libnetmgr.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libpsm_client.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libgps_default_so.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqdi.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_common_so.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmiidl.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libxml.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libtime_genoff.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/liblog.so.0.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libacdbrtac.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libadiertac.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libaudcal.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libaudioalsa.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libacdbmapper.so.1.0.0 ${D}/usr/lib/
      install -m 0644 ${S}/usr/lib/libqmi_sap.so.1.0.0 ${D}/usr/lib/

      # link services on boot
      ln -sf -r ${D}/etc/init.d/start_ipacm_perf_le ${D}/etc/rcS.d/start_ipacm_perf_le
      ln -sf -r ${D}/etc/init.d/start_shortcut_fe_le ${D}/etc/rcS.d/start_shortcut_fe_le
      ln -sf -r ${D}/etc/init.d/start_pdcd ${D}/etc/rcS.d/start_pdcd
      ln -sf -r ${D}/etc/init.d/start_sdcard ${D}/etc/rcS.d/start_sdcard
      ln -sf -r ${D}/etc/init.d/start_atfwd_daemon ${D}/etc/rcS.d/S80start_atfwd_daemon
      ln -sf -r ${D}/etc/init.d/start_QCMAP_ConnectionManager_le ${D}/etc/rcS.d/S39start_QCMAP_ConnectionManager_le
      ln -sf -r ${D}/etc/init.d/start_ipacmdiag_le ${D}/etc/rcS.d/start_ipacmdiag_le
      ln -sf -r ${D}/etc/init.d/init_irsc_util ${D}/etc/rcS.d/S29init_irsc_util
      ln -sf -r ${D}/etc/init.d/start_embms_le ${D}/etc/rcS.d/S35start_embms_le
      # ln -sf -r ${D}/etc/init.d/start_stop_qti_ppp_le ${D}/etc/rcS.d/start_stop_qti_ppp_le
      ln -sf -r ${D}/etc/init.d/qmuxd ${D}/etc/rcS.d/S40qmuxd
      ln -sf -r ${D}/etc/init.d/start_qti_le ${D}/etc/rcS.d/S38start_qti_le
      ln -sf -r ${D}/etc/init.d/start_qti_le ${D}/etc/rcS.d/K60start_qti_le
      ln -sf -r ${D}/etc/init.d/netmgrd ${D}/etc/rcS.d/S45netmgrd
      ln -sf -r ${D}/etc/init.d/psmd ${D}/etc/rcS.d/S36psmd

      ln -sf -r ${D}/etc/init.d/qmi_shutdown_modemd ${D}/etc/rcS.d/S45qmi_shutdown_modemd
      # Not really used. We'll see
      ln -sf -r ${D}/etc/init.d/start_alsa_daemon ${D}/etc/rcS.d/S99start_alsa_daemon 
# S45netmgrd S30mssboot
      # Symlink shared libraries
      ln -sf -r ${D}/usr/lib/libqcmaputils.so.1.0.0 ${D}/usr/lib/libqcmaputils.so.1
      # ln -sf -r ${D}/usr/lib/libpthread.so.0.0.0 ${D}/usr/lib/libpthread.so.0
      #ln -sf -r ${D}/usr/lib/libc.so.6.0.0 ${D}/usr/lib/libc.so.6
      ln -sf -r ${D}/usr/lib/libdiag.so.1.0.0 ${D}/usr/lib/libdiag.so.1
      ln -sf -r ${D}/usr/lib/libfaad2.so.1.0.0 ${D}/usr/lib/libfaad2.so.1
      ln -sf -r ${D}/usr/lib/libqmiservices.so.1.0.0 ${D}/usr/lib/libqmiservices.so.1
      ln -sf -r ${D}/usr/lib/libtts_data.so.1.0.0 ${D}/usr/lib/libtts_data.so.1
      ln -sf -r ${D}/usr/lib/libpugixml.so.1.0.0 ${D}/usr/lib/libpugixml.so.1
      ln -sf -r ${D}/usr/lib/libamrnb.so.1.0.0 ${D}/usr/lib/libamrnb.so.1
      ln -sf -r ${D}/usr/lib/libqmi_csi.so.1.0.0 ${D}/usr/lib/libqmi_csi.so.1
      ln -sf -r ${D}/usr/lib/libqcmapipc.so.1.0.0 ${D}/usr/lib/libqcmapipc.so.1
      ln -sf -r ${D}/usr/lib/libdsi_netctrl.so.0.0.0 ${D}/usr/lib/libdsi_netctrl.so.0
      ln -sf -r ${D}/usr/lib/libqmi_client_helper.so.1.0.0 ${D}/usr/lib/libqmi_client_helper.so.1
      ln -sf -r ${D}/usr/lib/libloc_stub.so.1.0.0 ${D}/usr/lib/libloc_stub.so.1
      ln -sf -r ${D}/usr/lib/librmnetctl.so.0.0.0 ${D}/usr/lib/librmnetctl.so.0
      ln -sf -r ${D}/usr/lib/libqmi_encdec.so.1.0.0 ${D}/usr/lib/libqmi_encdec.so.1
    #  ln -sf -r ${D}/usr/lib/librt.so.1.0.0 ${D}/usr/lib/librt.so.1
      ln -sf -r ${D}/usr/lib/libmad.so.1.0.0 ${D}/usr/lib/libmad.so.1
      ln -sf -r ${D}/usr/lib/libalsa_intf.so.1.0.0 ${D}/usr/lib/libalsa_intf.so.1
      ln -sf -r ${D}/usr/lib/libqcmap_cm.so.1.0.0 ${D}/usr/lib/libqcmap_cm.so.1
      ln -sf -r ${D}/usr/lib/libqmi_cci.so.1.0.0 ${D}/usr/lib/libqmi_cci.so.1
      ln -sf -r ${D}/usr/lib/libqmi.so.1.0.0 ${D}/usr/lib/libqmi.so.1
      ln -sf -r ${D}/usr/lib/libqmi_client_qmux.so.1.0.0 ${D}/usr/lib/libqmi_client_qmux.so.1
      ln -sf -r ${D}/usr/lib/libmp4ff.so.1.0.0 ${D}/usr/lib/libmp4ff.so.1
      #ln -sf -r ${D}/usr/lib/libgcc_s.so.1.0.0 ${D}/usr/lib/libgcc_s.so.1
      ln -sf -r ${D}/usr/lib/libflp_default.so.1.0.0 ${D}/usr/lib/libflp_default.so.1
      ln -sf -r ${D}/usr/lib/libconfigdb.so.0.0.0 ${D}/usr/lib/libconfigdb.so.0
      ln -sf -r ${D}/usr/lib/libdsutils.so.1.0.0 ${D}/usr/lib/libdsutils.so.1
      ln -sf -r ${D}/usr/lib/libfdk-aac.so.1.0.0 ${D}/usr/lib/libfdk-aac.so.1
      ln -sf -r ${D}/usr/lib/libacdbloader.so.1.0.0 ${D}/usr/lib/libacdbloader.so.1
      # ln -sf -r ${D}/usr/lib/libglib-2.0.so.0.0.0 ${D}/usr/lib/libglib-2.0.so.0
      ln -sf -r ${D}/usr/lib/libpsmutils.so.0.0.0 ${D}/usr/lib/libpsmutils.so.0
      ln -sf -r ${D}/usr/lib/libnetmgr.so.0.0.0 ${D}/usr/lib/libnetmgr.so.0
      ln -sf -r ${D}/usr/lib/libpsm_client.so.0.0.0 ${D}/usr/lib/libpsm_client.so.0
      ln -sf -r ${D}/usr/lib/libgps_default_so.so.1.0.0 ${D}/usr/lib/libgps_default_so.so.1
      ln -sf -r ${D}/usr/lib/libpsmutils.so.0.0.0 ${D}/usr/lib/libpsmutils.so.0
      ln -sf -r ${D}/usr/lib/libnetmgr.so.0.0.0 ${D}/usr/lib/libnetmgr.so.0
      ln -sf -r ${D}/usr/lib/libpsm_client.so.0.0.0 ${D}/usr/lib/libpsm_client.so.0
      ln -sf -r ${D}/usr/lib/libgps_default_so.so.1.0.0 ${D}/usr/lib/libgps_default_so.so.1
      ln -sf -r ${D}/usr/lib/libqdi.so.0.0.0 ${D}/usr/lib/libqdi.so.0
      ln -sf -r ${D}/usr/lib/libqmi_common_so.so.1.0.0 ${D}/usr/lib/libqmi_common_so.so.1
      ln -sf -r ${D}/usr/lib/libqmiidl.so.1.0.0 ${D}/usr/lib/libqmiidl.so.1
      ln -sf -r ${D}/usr/lib/libxml.so.0.0.0 ${D}/usr/lib/libxml.so.0
      ln -sf -r ${D}/usr/lib/libtime_genoff.so.1.0.0 ${D}/usr/lib/libtime_genoff.so.1
      ln -sf -r ${D}/usr/lib/liblog.so.0.0.0 ${D}/usr/lib/liblog.so.0
      ln -sf -r ${D}/usr/lib/libacdbrtac.so.1.0.0 ${D}/usr/lib/libacdbrtac.so.1
      ln -sf -r ${D}/usr/lib/libadiertac.so.1.0.0 ${D}/usr/lib/libadiertac.so.1
      ln -sf -r ${D}/usr/lib/libaudcal.so.1.0.0 ${D}/usr/lib/libaudcal.so.1
      ln -sf -r ${D}/usr/lib/libaudioalsa.so.1.0.0 ${D}/usr/lib/libaudioalsa.so.1
      ln -sf -r ${D}/usr/lib/libacdbmapper.so.1.0.0 ${D}/usr/lib/libacdbmapper.so.1
      ln -sf -r ${D}/usr/lib/libqmi_sap.so.1.0.0 ${D}/usr/lib/libqmi_sap.so.1

}
