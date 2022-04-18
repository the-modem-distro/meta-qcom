inherit bin_package

DESCRIPTION = "Scripts for rootfs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "sys-scripts"
MY_PN = "sys-scripts"
PR = "r0"
INSANE_SKIP_${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://find_partitions.sh"
S = "${WORKDIR}"


do_install() {
      install -d ${D}/etc/default
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/bin
      install -d ${D}/persist
      # Modem partition sits in firmware, but is linked in /lib/firmware/image
      # Since we cant expect the partition to be in place while building,
      # Just recreate the directory and link it
      install -d ${D}/firmware/image
      install -d ${D}/lib/firmware

      # We might as well make the data directory
      install -d ${D}/data

      install -m 0755  ${S}/find_partitions.sh ${D}/etc/init.d/
      
      ln -sf -r ${D}/etc/init.d/find_partitions.sh ${D}/etc/rcS.d/S10find_partitions.sh
      ln -sf -r ${D}/firmware/image ${D}/lib/firmware/image
      touch ${D}/etc/default/find_partitions.sh

}
