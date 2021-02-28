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
SRC_URI="file://init.d/usb \
         file://sbin/usb/target \
         file://sbin/usb/compositions/9001 \
         file://sbin/usb/compositions/empty \
         file://sbin/usb/compositions/F000 \
         file://sbin/usb/compositions/hsic_next \
         file://sbin/usb/compositions/hsusb_next"



S = "${WORKDIR}"

do_install() {
      #make folders if they dont exist
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/sbin/usb/compositions

      # USB Init script
      install -m 0755  ${S}/init.d/usb ${D}/etc/init.d
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rcS.d/S97usb

      # binaries
      install -m 0755  ${S}/sbin/usb/target ${D}/sbin/usb
      install -m 0755  ${S}/sbin/usb/compositions/9001 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/empty ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/F000 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/hsic_next ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/hsusb_next  ${D}/sbin/usb/compositions

      # USB init script uses a symlink in sbin/usb to finish configuring usb to set up the composition
      # Make the link so when init script starts finds it where it belongs
      # link services on boot
      ln -sf -r ${D}/sbin/usb/compositions/empty ${D}/sbin/usb/boot_hsic_composition
      ln -sf -r ${D}/sbin/usb/compositions/9001 ${D}/sbin/usb/boot_hsusb_composition
}
