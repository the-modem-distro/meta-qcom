inherit bin_package

DESCRIPTION = "Quectel Proprietary blobs and scripts"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "quectel-proprietary"
RDEPENDS_${PN} = "libcap"
PACKAGE_WRITE_DEPS = "libcap-native"
MY_PN = "quectel-proprietary"
PR = "r1"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://bin/ql_forward \
         file://bin/ql_manager_cli \
         file://bin/ql_manager_server \
         file://bin/start_quec_daemon \
         file://bin/start_quec_pcm_daemon \
         file://bin/quectel_daemon \
         file://bin/quectel_monitor_daemon \
         file://bin/quectel_pcm_daemon \
         file://bin/quectel_psm_aware \
         file://bin/quectel_tts_service \
         file://bin/quectel-gps-handle \
         file://bin/quectel-remotefs-service \
         file://bin/quectel-smd-atcmd \
         file://bin/quectel-thermal \
         file://bin/quectel-uart-ddp \
         file://bin/xtwifi-inet-agent \
         file://bin/xtwifi-client \
         file://bin/lowi-server \
         file://bin/loc_launcher \
         file://init.d/start_ql_forward_le \
         file://init.d/start_ql_manager_server_le \ 
         file://init.d/qmi_shutdown_modemd \ 
         file://init.d/quectel_daemon \ 
         file://init.d/quectel_psm_aware \ 
         file://init.d/quectel-gps-handle \ 
         file://init.d/quectel-monitor-daemon \ 
         file://init.d/quectel-smd-atcmd \ 
         file://init.d/quectel-thermal \ 
         file://init.d/quectel-uart-ddp \
         file://init.d/start_loc_launcher \ 
         file://init.d/start_quectel_remotefs_daemon \ 
         file://init.d/start_quectel_pcm_daemon \ 
         file://etc/xtwifi.conf \
         file://etc//ql_manager_network.conf \
         file://lib/libssl.so.1.0.0 \
         file://lib/libcrypto.so.1.0.0 \
         file://lib/libasn1c_cper.so.1.0.0 \
         file://lib/libasn1c_crt.so.1.0.0 \
         file://lib/libasn1c_rtx.so.1.0.0 \
         file://lib/libgdtap_adapter.so.1.0.0 \
         file://lib/libizat_core.so.1.0.0 \
         file://lib/libloc_base_util.so.1.0.0 \
         file://lib/libloc_api_v02.so.1.0.0 \
         file://lib/liblowi_client.so.1.0.0 \
         file://lib/libxtwifi_tile.so.1.0.0 \
         file://lib/libdataitems.so.1.0.0 \
         file://lib/libutils.so.0.0.0 \
         file://lib/libnl-genl-3.so.200 \
         file://lib/libsqlite3.so.0.8.6 \
         file://lib/libxtwifiserver_protocol_uri_v3.so.1.0.0 \
         file://lib/libxtwifiserver_protocol.so.1.0.0"

S = "${WORKDIR}/"

do_install() {
      #make folders if they dont exist
      install -d ${D}/persist
      install -d ${D}/usr/bin
      install -d ${D}/usr/lib
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/etc/quectel/ql_manager
      # binaries
 #     install -m 0755  ${S}/bin/ql_usbcfg ${D}/usr/bin
      install -m 0755  ${S}/bin/loc_launcher ${D}/usr/bin
      install -m 0755  ${S}/bin/ql_forward ${D}/usr/bin
      install -m 0755  ${S}/bin/ql_manager_cli ${D}/usr/bin
      install -m 0755  ${S}/bin/ql_manager_server ${D}/usr/bin
      install -m 0755  ${S}/bin/start_quec_daemon ${D}/usr/bin
      install -m 0755  ${S}/bin/start_quec_pcm_daemon ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel_daemon ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel_monitor_daemon ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel_pcm_daemon ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel_psm_aware ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel_tts_service ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel-gps-handle ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel-remotefs-service ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel-smd-atcmd ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel-thermal ${D}/usr/bin
      install -m 0755  ${S}/bin/quectel-uart-ddp ${D}/usr/bin
      install -m 0755  ${S}/bin/xtwifi-client ${D}/usr/bin
      install -m 0755  ${S}/bin/xtwifi-inet-agent ${D}/usr/bin
      install -m 0755  ${S}/bin/lowi-server ${D}/usr/bin
      install -m 0644  ${S}/etc/xtwifi.conf ${D}/etc
      install -m 0644  ${S}/etc/ql_manager_network.conf ${D}/etc/quectel/ql_manager

      # services
      install -m 0755  ${S}/init.d/start_loc_launcher ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/start_ql_forward_le ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/start_ql_manager_server_le ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/qmi_shutdown_modemd ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel_daemon ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel_psm_aware ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel-gps-handle ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel-monitor-daemon ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel-smd-atcmd ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel-thermal ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/quectel-uart-ddp ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/start_quectel_remotefs_daemon ${D}/etc/init.d/
      install -m 0755  ${S}/init.d/start_quectel_pcm_daemon ${D}/etc/init.d/

      # location depended libraries. Something something izatcloud.net
      # libcrypto 1.0.0 is needed to get OPENSSL1.0, otherwise everything complains
      install -m 0644  ${S}/lib/libssl.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libnl-genl-3.so.200 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libcrypto.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libdataitems.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libutils.so.0.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libloc_api_v02.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libasn1c_cper.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libasn1c_crt.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libasn1c_rtx.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libgdtap_adapter.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libizat_core.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libloc_base_util.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/liblowi_client.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libxtwifi_tile.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libxtwifiserver_protocol_uri_v3.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libxtwifiserver_protocol.so.1.0.0 ${D}/usr/lib/
      install -m 0644  ${S}/lib/libsqlite3.so.0.8.6 ${D}/usr/lib/

      # link services on boot
      ln -sf -r ${D}/etc/init.d/start_loc_launcher ${D}/etc/rcS.d/S99start_loc_launcher
      ln -sf -r ${D}/etc/init.d/start_ql_manager_server_le ${D}/etc/rcS.d/S99start_ql_manager_server_le
      ln -sf -r ${D}/etc/init.d/start_ql_forward_le ${D}/etc/rcS.d/S45start_ql_forward_le
      #ln -sf -r ${D}/etc/init.d/qmi_shutdown_modemd ${D}/etc/rcS.d/S45qmi_shutdown_modemd
      ln -sf -r ${D}/etc/init.d/quectel_daemon ${D}/etc/rcS.d/S45quectel_daemon
      ln -sf -r ${D}/etc/init.d/quectel_psm_aware ${D}/etc/rcS.d/S45quectel_psm_aware
      ln -sf -r ${D}/etc/init.d/quectel-gps-handle ${D}/etc/rcS.d/S45quectel-gps-handle
      ln -sf -r ${D}/etc/init.d/quectel-monitor-daemon ${D}/etc/rcS.d/S45quectel-monitor-daemon
      ln -sf -r ${D}/etc/init.d/quectel-smd-atcmd ${D}/etc/rcS.d/S45quectel-smd-atcmd 
      ln -sf -r ${D}/etc/init.d/quectel-thermal ${D}/etc/rcS.d/S45quectel-thermal
      ln -sf -r ${D}/etc/init.d/quectel-uart-ddp ${D}/etc/rcS.d/S45quectel-uart-ddp
      ln -sf -r ${D}/etc/init.d/start_quectel_pcm_daemon ${D}/etc/rcS.d/S45start_quectel_pcm_daemon
      ln -sf -r ${D}/etc/init.d/start_quectel_remotefs_daemon ${D}/etc/rcS.d/S45start_quectel_remotefs_daemon

      ln -sf -r ${D}/usr/lib/libloc_api_v02.so.1.0.0 ${D}/usr/lib/libasn1c_cper.so.1
      ln -sf -r ${D}/usr/lib/libasn1c_cper.so.1.0.0 ${D}/usr/lib/libasn1c_cper.so.1
      ln -sf -r ${D}/usr/lib/libasn1c_crt.so.1.0.0 ${D}/usr/lib/libasn1c_crt.so.1
      ln -sf -r ${D}/usr/lib/libasn1c_rtx.so.1.0.0 ${D}/usr/lib/libasn1c_rtx.so.1
      ln -sf -r ${D}/usr/lib/libgdtap_adapter.so.1.0.0 ${D}/usr/lib/libgdtap_adapter.so.1
      ln -sf -r ${D}/usr/lib/libizat_core.so.1.0.0 ${D}/usr/lib/libizat_core.so.1
      ln -sf -r ${D}/usr/lib/libloc_base_util.so.1.0.0 ${D}/usr/lib/libloc_base_util.so.1
      ln -sf -r ${D}/usr/lib/liblowi_client.so.1.0.0 ${D}/usr/lib/liblowi_client.so.1
      ln -sf -r ${D}/usr/lib/libxtwifi_tile.so.1.0.0 ${D}/usr/lib/libxtwifi_tile.so.1
      ln -sf -r ${D}/usr/lib/libxtwifiserver_protocol_uri_v3.so.1.0.0 ${D}/usr/lib/libxtwifiserver_protocol_uri_v3.so.1
      ln -sf -r ${D}/usr/lib/libxtwifiserver_protocol.so.1.0.0 ${D}/usr/lib/libxtwifiserver_protocol.so.1
      ln -sf -r ${D}/usr/lib/libdataitems.so.1.0.0 ${D}/usr/lib/libdataitems.so.1
      ln -sf -r ${D}/usr/lib/libutils.so.0.0.0 ${D}/usr/lib/libutils.so.0
      ln -sf -r ${D}/usr/lib/libsqlite3.so.0.8.6 ${D}/usr/lib/libsqlite3.so.0
}


pkg_postinst_${PN}() {
      setcap cap_net_raw+ep "$D/usr/bin/loc_launcher"
      setcap cap_net_raw+ep "$D/usr/bin/ql_forward"
      setcap cap_net_raw+ep "$D/usr/bin/ql_manager_cli"
      setcap cap_net_raw+ep "$D/usr/bin/ql_manager_server"
      setcap cap_net_raw+ep "$D/usr/bin/quectel_daemon"
      setcap cap_net_raw+ep "$D/usr/bin/quectel_monitor_daemon"
      setcap cap_net_raw+ep "$D/usr/bin/quectel_pcm_daemon"
      setcap cap_net_raw+ep "$D/usr/bin/quectel_psm_aware"
      setcap cap_net_raw+ep "$D/usr/bin/quectel_tts_service"
      setcap cap_net_raw+ep "$D/usr/bin/quectel-gps-handle"
      setcap cap_net_raw+ep "$D/usr/bin/quectel-remotefs-service"
      setcap cap_net_raw+ep "$D/usr/bin/quectel-smd-atcmd"
      #setcap cap_net_raw+ep "$D/usr/bin/quectel-thermal"
      setcap cap_net_raw+ep "$D/usr/bin/quectel-uart-ddp"
      setcap cap_net_raw+ep "$D/usr/bin/xtwifi-client"
      setcap cap_net_raw+ep "$D/usr/bin/xtwifi-inet-agent"
      setcap cap_net_raw+ep "$D/usr/bin/lowi-server"
}