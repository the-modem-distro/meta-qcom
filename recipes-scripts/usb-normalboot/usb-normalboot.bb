inherit bin_package

DESCRIPTION = "Scripts to autostart USB in rootfs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "usb-normalboot"
MY_PN = "usb-normalboot"
PR = "r1"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://init.d/usb"

S = "${WORKDIR}"

do_install() {
      #make folders if they dont exist
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d

      # USB Init script
      install -m 0755  ${S}/init.d/usb ${D}/etc/init.d
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rcS.d/S97usb
}
