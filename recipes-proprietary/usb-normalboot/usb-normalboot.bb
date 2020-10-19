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
         file://sbin/usb/compositions/90A1 \
         file://sbin/usb/compositions/90A9 \
         file://sbin/usb/compositions/90AD \
         file://sbin/usb/compositions/90B1 \
         file://sbin/usb/compositions/901D \
         file://sbin/usb/compositions/902B \
         file://sbin/usb/compositions/902D \
         file://sbin/usb/compositions/904A \
         file://sbin/usb/compositions/905B \
         file://sbin/usb/compositions/9000 \
         file://sbin/usb/compositions/9001 \
         file://sbin/usb/compositions/9002 \
         file://sbin/usb/compositions/9003 \
         file://sbin/usb/compositions/9004 \
         file://sbin/usb/compositions/9005 \
         file://sbin/usb/compositions/9006 \
         file://sbin/usb/compositions/9007 \
         file://sbin/usb/compositions/9011 \
         file://sbin/usb/compositions/9016 \
         file://sbin/usb/compositions/9018 \
         file://sbin/usb/compositions/9021 \
         file://sbin/usb/compositions/9021 \
         file://sbin/usb/compositions/9024 \
         file://sbin/usb/compositions/9025 \
         file://sbin/usb/compositions/9049 \
         file://sbin/usb/compositions/9056 \
         file://sbin/usb/compositions/9057 \
         file://sbin/usb/compositions/9059 \
         file://sbin/usb/compositions/9060 \
         file://sbin/usb/compositions/9063 \
         file://sbin/usb/compositions/9064 \
         file://sbin/usb/compositions/9067 \
         file://sbin/usb/compositions/9084 \
         file://sbin/usb/compositions/9085 \
         file://sbin/usb/compositions/9091 \
         file://sbin/usb/compositions/empty \
         file://sbin/usb/compositions/F000 \
         file://sbin/usb/compositions/hsic_next \
         file://sbin/usb/compositions/hsusb_next"



S = "${WORKDIR}/"

do_install() {
      #make folders if they dont exist
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/sbin/usb/compositions

      # USB Init script
      install -m 0755  ${S}/init.d/usb ${D}/etc/init.d
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rcS.d/S90usb

      # binaries
      install -m 0755  ${S}/sbin/usb/target ${D}/sbin/usb
      install -m 0755  ${S}/sbin/usb/compositions/90A1 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/90A9 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/90AD ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/90B1 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/901D ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/902B ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/902D ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/904A ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/905B ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9000 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9001 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9002 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9003 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9004 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9005 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9006 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9007 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9011 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9016 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9018 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9021 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9021 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9024 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9025 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9049 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9056 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9057 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9059 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9060 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9063 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9064 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9067 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9084 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9085 ${D}/sbin/usb/compositions
      install -m 0755  ${S}/sbin/usb/compositions/9091 ${D}/sbin/usb/compositions
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
