inherit bin_package

DESCRIPTION = "USB init script to launch adb"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "usb-recoveryboot"
MY_PN = "usb-recoveryboot"
PR = "r0"
INSANE_SKIP_${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://usb"
S = "${WORKDIR}/"


do_install() {
      install -d ${D}/etc/default
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d

      install -m 0755  ${S}/usb ${D}/etc/init.d/

      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rcS.d/S98usb
      touch ${D}/etc/default/usb

}
