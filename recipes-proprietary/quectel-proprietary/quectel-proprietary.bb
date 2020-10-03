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
SRC_URI="file://ql_forward \
         file://ql_manager_cli \
         file://ql_manager_server \
         file://start_ql_forward_le \
         file://start_ql_manager_server_le"


S = "${WORKDIR}/"

do_install() {
      #make folders if they dont exist
      install -d ${D}/usr/bin
      install -d ${D}/usr/lib
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      # binaries
      cp  ${S}/ql_forward ${D}/usr/bin
      cp  ${S}/ql_manager_cli ${D}/usr/bin
      cp  ${S}/ql_manager_server ${D}/usr/bin
      # services
      cp  ${S}/start_ql_forward_le ${D}/etc/init.d/
      cp  ${S}/start_ql_manager_server_le ${D}/etc/init.d/

      #set executable flags
      chmod +x ${D}/usr/bin/ql_forward
      chmod +x ${D}/usr/bin/ql_manager_server
      chmod +x ${D}/usr/bin/ql_manager_cli

      chmod +x ${D}/etc/init.d/start_ql_forward_le
      chmod +x ${D}/etc/init.d/start_ql_manager_server_le
      # link services on boot
      ln -sf -r ${D}/etc/init.d/start_ql_manager_server_le ${D}/etc/rcS.d/S80start_ql_manager_server_le
      ln -sf -r ${D}/etc/init.d/start_ql_forward_le ${D}/etc/rcS.d/S99start_ql_forward_le
      
      #link libraries
}
