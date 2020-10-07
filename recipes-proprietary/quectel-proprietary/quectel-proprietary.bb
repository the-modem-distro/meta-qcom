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
SRC_URI="file://bin/ql_forward \
         file://bin/ql_manager_cli \
         file://bin/ql_manager_server \
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
         file://init.d/start_ql_forward_le \
         file://init.d/start_ql_manager_server_le \ 
         file://init.d/qmi_shutdown_modemd \ 
         file://init.d/quectel_daemon \ 
         file://init.d/quectel_psm_aware \ 
         file://init.d/quectel-gps-handle \ 
         file://init.d/quectel-monitor-daemon \ 
         file://init.d/quectel-smd-atcmd \ 
         file://init.d/quectel-thermal \ 
         file://init.d/quectel-uart-ddp"


S = "${WORKDIR}/"

do_install() {
      #make folders if they dont exist
      install -d ${D}/usr/bin
      install -d ${D}/usr/lib
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      # binaries
      install -m 0755  ${S}/bin/ql_forward ${D}/usr/bin
      install -m 0755  ${S}/bin/ql_manager_cli ${D}/usr/bin
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

      # services
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

      # link services on boot
      ln -sf -r ${D}/etc/init.d/start_ql_manager_server_le ${D}/etc/rcS.d/S80start_ql_manager_server_le
      ln -sf -r ${D}/etc/init.d/start_ql_forward_le ${D}/etc/rcS.d/S99start_ql_forward_le
      ln -sf -r ${D}/etc/init.d/qmi_shutdown_modemd ${D}/etc/rcS.d/S45qmi_shutdown_modemd
      ln -sf -r ${D}/etc/init.d/quectel_daemon ${D}/etc/rcS.d/S45quectel_daemon
      ln -sf -r ${D}/etc/init.d/quectel_psm_aware ${D}/etc/rcS.d/S45quectel_psm_aware
      ln -sf -r ${D}/etc/init.d/quectel-gps-handle ${D}/etc/rcS.d/S45quectel-gps-handle
      ln -sf -r ${D}/etc/init.d/quectel-monitor-daemon ${D}/etc/rcS.d/S45quectel-monitor-daemon
      ln -sf -r ${D}/etc/init.d/quectel-smd-atcmd ${D}/etc/rcS.d/S45quectel-smd-atcmd 
      ln -sf -r ${D}/etc/init.d/quectel-thermal ${D}/etc/rcS.d/S45quectel-thermal
      ln -sf -r ${D}/etc/init.d/quectel-uart-ddp ${D}/etc/rcS.d/S45quectel-uart-ddp
      
}
