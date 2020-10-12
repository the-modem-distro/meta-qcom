inherit bin_package

DESCRIPTION = "Shared scripts for rootfs and recoveryfs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "shared-scripts"
MY_PN = "shared-scripts"
PR = "r0"
INSANE_SKIP_${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://adbd \
         file://find_partitions.sh \
         file://recoverymount \
         file://chgrp-diag \
         file://rootfsmount "
S = "${WORKDIR}/"


do_install() {
      install -d ${D}/etc/default
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/bin
      # Modem partition sits in firmware, but is linked in /lib/firmware/image
      # Since we cant expect the partition to be in place while building,
      # Just recreate the directory and link it
      install -d ${D}/firmware/image
      install -d ${D}/lib/firmware

      # We might as well make the data directory
      install -d ${D}/data_swap
      install -d ${D}/data
      install -d ${D}/cache

      install -m 0755 ${S}/adbd ${D}/etc/init.d/
      install -m 0755  ${S}/find_partitions.sh ${D}/etc/init.d/
      install -m 0755  ${S}/chgrp-diag ${D}/etc/init.d/
      install -m 0755  ${S}/recoverymount ${D}/bin
      install -m 0755  ${S}/rootfsmount ${D}/bin
      
      ln -sf -r ${D}/etc/init.d/adbd ${D}/etc/rcS.d/S99adbd
      ln -sf -r ${D}/etc/init.d/find_partitions.sh ${D}/etc/rcS.d/S20find_partitions.sh
      ln -sf -r ${D}/etc/init.d/chgrp-diag ${D}/etc/rcS.d/S10chgrp-diag
      ln -sf -r ${D}/firmware/image ${D}/lib/firmware/image
      touch ${D}/etc/default/adbd
      touch ${D}/etc/default/find_partitions.sh

}
