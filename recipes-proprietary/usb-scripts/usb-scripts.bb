inherit bin_package

DESCRIPTION = "USB and ADB init scripts"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "usb-scripts"
MY_PN = "usb-scripts"
PR = "r0"
INSANE_SKIP_${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://usb \
         file://adbd \
         file://find_partitions.sh"
S = "${WORKDIR}/"


do_install() {
      install -d ${D}/etc/default
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rc0.d
      install -d ${D}/etc/rc1.d
      install -d ${D}/etc/rc2.d
      install -d ${D}/etc/rc3.d
      install -d ${D}/etc/rc4.d
      install -d ${D}/etc/rc5.d
      cp  ${S}/usb ${D}/etc/init.d/
      cp  ${S}/adbd ${D}/etc/init.d/
      cp  ${S}/find_partitions.sh ${D}/etc/init.d/
      chmod +x ${D}/etc/init.d/usb
      chmod +x ${D}/etc/init.d/adbd
      chmod +x ${D}/etc/init.d/find_partitions.sh
      cd ${D}/etc/init.d/
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc0.d/S80usb
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc1.d/S80usb
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc2.d/S80usb
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc3.d/S80usb
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc4.d/S80usb
      ln -sf -r ${D}/etc/init.d/usb ${D}/etc/rc5.d/S80usb


      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc0.d/S99adbd
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc1.d/S99adbd
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc2.d/S99adbd
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc3.d/S99adbd
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc4.d/S99adbd
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc5.d/S99adbd

      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc0.d/S81find_partitions.sh
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc1.d/S81find_partitions.sh
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc2.d/S81find_partitions.sh
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc3.d/S81find_partitions.sh
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc4.d/S81find_partitions.sh
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rc5.d/S81find_partitions.sh

      touch ${D}/etc/default/usb
      touch ${D}/etc/default/adbd
      touch ${D}/etc/default/find_partitions.sh

}
